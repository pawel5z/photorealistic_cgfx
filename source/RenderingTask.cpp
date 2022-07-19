#include "RenderingTask.hpp"

#include "BRDFs.hpp"
#include "ogl_interface/Axes.hpp"
#include "utils.hpp"

#include "indicators/progress_bar.hpp"

#include <OpenEXR/ImfChannelList.h>
#include <OpenEXR/ImfFrameBuffer.h>
#include <OpenEXR/ImfHeader.h>
#include <OpenEXR/ImfOutputFile.h>
#include <assimp/Importer.hpp>  // C++ importer interface
#include <assimp/postprocess.h> // Post processing flags
#include <assimp/scene.h>       // Output data structure
#include <glm/gtx/intersect.hpp>
#include <glm/gtx/norm.hpp>

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <numeric>

using namespace std::chrono_literals;
using namespace std::string_literals;
namespace fs = std::filesystem;

CacheAlignedCounter::CacheAlignedCounter(unsigned int counter) : counter(counter) {}

RenderingTask::RenderingTask(std::string rtcPath, unsigned int nSamples, unsigned int concThreads)
    : rtcPath(rtcPath), nSamples(nSamples) {
    this->concThreads = std::max(1U, std::min(std::thread::hardware_concurrency(), concThreads));

    std::cerr << "Reading data...\n";

    std::ifstream configFile(rtcPath);
    if (!configFile.is_open()) {
        std::cerr << "Could not open '" + rtcPath + "'.\n";
        exit(EXIT_FAILURE);
    }

    fs::path rtcDir = fs::path(rtcPath).parent_path();
    std::string line;

    std::getline(configFile, line); // ignore comment line

    std::getline(configFile, origObjPath);
    std::string objPath = origObjPath;
    objPath = fs::path(rtcDir).append(objPath).string();

    std::getline(configFile, outputPath);

    std::getline(configFile, line);
    try {
        recLvl = std::stoi(line);
    } catch (const std::exception &e) {
        std::cerr << "Could not parse recursion level parameter.\n";
        throw e;
    }

    std::getline(configFile, line);
    if (sscanf(line.c_str(), "%u %u", &width, &height) < 2) {
        std::cerr << "Could not parse resolution.\n";
        exit(EXIT_FAILURE);
    }

    std::getline(configFile, line);
    if (sscanf(line.c_str(), "%f %f %f", &viewPoint.x, &viewPoint.y, &viewPoint.z) < 3) {
        std::cerr << "Could not parse View Point.\n";
        exit(EXIT_FAILURE);
    }

    std::getline(configFile, line);
    if (sscanf(line.c_str(), "%f %f %f", &lookAt.x, &lookAt.y, &lookAt.z) < 3) {
        std::cerr << "Could not parse Look At.\n";
        exit(EXIT_FAILURE);
    }

    std::getline(configFile, line);
    if (sscanf(line.c_str(), "%f %f %f", &up.x, &up.y, &up.z) < 3) {
        std::cerr << "Could not parse Vector Up. Using default (0, 1, 0).\n";
    }

    std::getline(configFile, line);
    try {
        yView = std::stof(line);
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n' << "Could not parse yview. Using default 1.0.\n";
    }

    recomputeCameraParams();

    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(objPath, aiProcess_Triangulate | aiProcess_GenNormals |
                                                          aiProcess_FixInfacingNormals |
                                                          aiProcess_JoinIdenticalVertices);
    if (scene == nullptr) {
        std::cerr << importer.GetErrorString() << '\n';
        throw std::logic_error("Error while importing \"" + objPath + "\".\n");
    }

    for (int i = 0; i < scene->mNumMaterials; i++)
        mats.emplace_back(scene->mMaterials[i]);

    bool foundIncorrectNormals = false;
    for (int i = 0; i < scene->mNumMeshes; i++) {
        aiMesh *mesh = scene->mMeshes[i];

        meshes.emplace_back(triangles.size(), mesh->mNumFaces, mesh->mMaterialIndex);

        for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
            aiFace f = mesh->mFaces[i];
            Triangle t;
            for (unsigned int j = 0; j < f.mNumIndices; j++)
                t.indices[j] = f.mIndices[j] + vertices.size();
            triangles.push_back(t);
            trianglesToMatIndices.push_back(mesh->mMaterialIndex);
        }

        for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
            aiVector3D v = mesh->mVertices[i];
            aiVector3D n = mesh->mNormals[i];
            glm::vec3 norm = glm::normalize(glm::vec3(n.x, n.y, n.z));
            if (std::isnan(norm.x) || std::isnan(norm.y) || std::isnan(norm.z))
                foundIncorrectNormals = true;
            vertices.push_back({{v.x, v.y, v.z}, norm});
        }
    }
    if (foundIncorrectNormals)
        std::cerr << "Some normals are incorrect: zero or nan.\n";

    for (unsigned int tIdx = 0; tIdx < triangles.size(); tIdx++) {
        glm::vec3 ke = mats.at(trianglesToMatIndices.at(tIdx)).ke;
        if (ke.r != 0.f || ke.g != 0.f || ke.b != 0.f)
            lightIndices.push_back(tIdx);
    }

    lightPowersCombined = 0.f;
    for (unsigned int lightTrianIdx : lightIndices) {
        const Material &mat = mats.at(trianglesToMatIndices.at(lightTrianIdx));
        float power =
            (mat.ke.r + mat.ke.g + mat.ke.b) * triangles.at(lightTrianIdx).area(vertices) / 3.f;
        lightPowersCdf.push_back(lightPowersCdf.size() ? power + lightPowersCdf.back() : power);
        lightPowersCombined += power;
    }

    std::cerr << triangles.size() << " triangles\n";
}

void RenderingTask::render() const {
    std::vector<std::vector<glm::vec3>> pixels(height, std::vector<glm::vec3>(width, glm::vec3(0)));
    std::vector<CacheAlignedCounter> progress(concThreads);
    std::vector<std::thread> ts;

    /* Distribute pixels randomly between threads in order to make each thread
     * work on average the same amount of time. */
    std::unique_ptr<std::vector<unsigned int>> flatCoords(
        new std::vector<unsigned int>(width * height, 0u));
    std::iota(flatCoords->begin(), flatCoords->end(), 0u);
#ifdef DEBUG
    std::shuffle(flatCoords->begin(), flatCoords->end(), std::mt19937(42));
#else
    std::shuffle(flatCoords->begin(), flatCoords->end(), std::mt19937((std::random_device())()));
#endif // DEBUG
    std::vector<std::queue<unsigned int>> flatCoordsQueues(concThreads, std::queue<unsigned int>());
    for (unsigned int i = 0; i != flatCoords->size();)
        for (unsigned int j = 0; j < concThreads && i != flatCoords->size(); j++) {
            flatCoordsQueues.at(j).push(flatCoords->at(i));
            i++;
        }
    flatCoords.reset();

    std::mutex endLock;
    auto begin = std::chrono::steady_clock::now();
    auto end = begin;
    for (unsigned int i = 0; i < concThreads; i++)
        ts.emplace_back(&RenderingTask::renderBatch, this, std::ref(pixels),
                        std::move(flatCoordsQueues.at(i)), std::ref(progress.at(i)), std::ref(end),
                        std::ref(endLock));
    std::cout << "Rendering using " << concThreads << " thread" << (concThreads == 1 ? "" : "s")
              << "...\n";
    std::this_thread::yield();
    indicators::ProgressBar bar(indicators::option::ShowElapsedTime(true),
                                indicators::option::ShowRemainingTime(true),
                                indicators::option::ShowPercentage(true),
                                indicators::option::FontStyles(std::vector<indicators::FontStyle>{
                                    indicators::FontStyle::bold}));
    while (true) {
        unsigned int progressSoFar = 0;
        for (const auto &counter : progress)
            progressSoFar += counter.counter;
        bar.set_progress(progressSoFar * 100 / (width * height));
        if (progressSoFar == width * height)
            break;
        std::this_thread::sleep_for(1s);
    }
    for (auto &t : ts)
        t.join();

    float tracingTime =
        std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count() / 1000000.f;
    std::cout << "Rendering time: " << tracingTime << " seconds.\n";

    // saving to file
    std::array<std::vector<half>, 3> imgData{std::vector<half>(width * height, 0),
                                             std::vector<half>(width * height, 0),
                                             std::vector<half>(width * height, 0)};
    for (unsigned int py = 0; py < height; py++)
        for (unsigned int px = 0; px < width; px++)
            for (unsigned int i = 0; i < 3; i++)
                imgData.at(i).at(py * width + px) = pixels.at(py).at(px)[i];
    Imf::Header header(width, height);
    header.channels().insert("R", Imf::Channel(Imf::HALF));
    header.channels().insert("G", Imf::Channel(Imf::HALF));
    header.channels().insert("B", Imf::Channel(Imf::HALF));
    Imf::OutputFile file(outputPath.c_str(), header);
    Imf::FrameBuffer frameBuffer;
    frameBuffer.insert("R", Imf::Slice(Imf::HALF, (char *)imgData.at(0).data(),
                                       sizeof(imgData.at(0).at(0)),
                                       sizeof(imgData.at(0).at(0)) * width));
    frameBuffer.insert("G", Imf::Slice(Imf::HALF, (char *)imgData.at(1).data(),
                                       sizeof(imgData.at(1).at(0)),
                                       sizeof(imgData.at(1).at(0)) * width));
    frameBuffer.insert("B", Imf::Slice(Imf::HALF, (char *)imgData.at(2).data(),
                                       sizeof(imgData.at(2).at(0)),
                                       sizeof(imgData.at(2).at(0)) * width));
    file.setFrameBuffer(frameBuffer);
    file.writePixels(height);
}

void RenderingTask::preview() {
    RTWindow window(this);
    window.Init(1280, 720, "Preview");
    window.MainLoop();
}

std::ostream &operator<<(std::ostream &os, const RenderingTask *rt) {
    os << "#\n"
       << rt->origObjPath << '\n'
       << rt->outputPath << '\n'
       << rt->recLvl << '\n'
       << rt->width << ' ' << rt->height << '\n'
       << rt->viewPoint.x << ' ' << rt->viewPoint.y << ' ' << rt->viewPoint.z << '\n'
       << rt->lookAt.x << ' ' << rt->lookAt.y << ' ' << rt->lookAt.z << '\n'
       << rt->up.x << ' ' << rt->up.y << ' ' << rt->up.z << '\n'
       << rt->yView;
    return os;
}

void RenderingTask::updateRTCFile() {
    std::ofstream fout(rtcPath);
    fout << this;
}

void RenderingTask::buildAccStructures() {
    std::cerr << "Building acceleration structure...\n";
    kdTree = std::unique_ptr<KDTree>(new KDTree(triangles, vertices,
                                                std::round(8 + 1.3f * std::log2(triangles.size())),
                                                16, 0.f, 1.f, 80.f));
}

Ray RenderingTask::getPrimaryRay(unsigned int px, unsigned int py) const {
    return {viewPoint, glm::normalize(front + up * -((float)py * 2.f / (float)(height - 1) - 1.f) +
                                      right * ((float)px * 2.f / (float)(width - 1) - 1.f))};
}

glm::vec3
RenderingTask::traceRay(const Ray &r, unsigned int maxDepth, std::mt19937 &randEng,
                        const std::function<glm::vec3(const glm::vec3 &, const glm::vec3 &,
                                                      const glm::vec3 &, const Material &)> &brdf,
                        HemisphereSampler &sampler) const {
    float t;
    glm::vec3 n;
    const Material *mat;
    if (maxDepth == 0 || !findNearestIntersection(r, t, n, &mat))
        return {0, 0, 0};

    glm::vec3 color(0);
    if (mat->ke.r > 0.f || mat->ke.g > 0.f || mat->ke.b > 0.f) {
        if (maxDepth == recLvl)
            color = mat->ke / glm::pi<float>();
        return color;
    }

    glm::vec3 hit = r.o + t * r.d;
    static thread_local std::uniform_real_distribution<float> lightPowersDist(0.f,
                                                                              lightPowersCombined);
    static thread_local std::uniform_real_distribution<float> uniDist;
    // sample random light if possible
    if (lightIndices.size() != 0) {
        unsigned int lightIdx = getLightIdxFromRndVal(lightPowersDist(randEng));
        const Triangle &light = triangles.at(lightIdx);
        float alpha = uniDist(randEng);
        float beta = 1.f - alpha;
        const Vertex &lA = vertices.at(light.indices[0]);
        const Vertex &lB = vertices.at(light.indices[1]);
        const Vertex &lC = vertices.at(light.indices[2]);
        glm::vec3 randLightPoint = lA.pos + alpha * (lB.pos - lA.pos) + beta * (lC.pos - lA.pos);
        Ray lightRay(hit, glm::normalize(randLightPoint - hit));
        glm::vec3 lightNorm =
            glm::normalize(lA.norm + alpha * (lB.norm - lA.norm) + beta * (lC.norm - lA.norm));
        float lightSqDist = glm::distance2(hit, randLightPoint);
        float lightArea = light.area(vertices);
        if (!isObstructed(lightRay, randLightPoint) && lightSqDist > minLightSqDist) {
            const Material &lightMat = mats.at(trianglesToMatIndices.at(lightIdx));
            float lightProb = lightArea * (lightMat.ke.r + lightMat.ke.g + lightMat.ke.b) / 3.f /
                              lightPowersCombined;
            if (lightProb > .01f)
                color += lightMat.ke * lightArea * brdf(lightRay.d, -r.d, n, *mat) *
                         glm::abs(glm::dot(n, lightRay.d) * glm::dot(lightNorm, -lightRay.d)) /
                         lightProb / lightSqDist;
        }
    }

    float russianRouletteAlpha =
        (mat->kd.r + mat->kd.g + mat->kd.b + mat->ks.r + mat->ks.g + mat->ks.b) / 3.f;
    if (uniDist(randEng) <= russianRouletteAlpha) {
        // sample random incoming vector
        auto [s, prob] = sampler(mat);
        if (prob < .01f)
            return color;
        Ray incoming(hit, sampler.makeSampleRelativeToNormal(s, n));
        color += brdf(incoming.d, -r.d, n, *mat) *
                 traceRay(incoming, maxDepth - 1, randEng, brdf, sampler) *
                 glm::abs(glm::dot(n, incoming.d)) / (prob * russianRouletteAlpha);
    }

    return color;
}

bool RenderingTask::findNearestIntersection(const Ray &r, float &t, glm::vec3 &n,
                                            const Material **mat) const {
    unsigned int trianIdx;
    bool ret = kdTree->findNearestIntersection(Ray(r), triangles, vertices, t, n, trianIdx);
    if (ret)
        *mat = &mats.at(trianglesToMatIndices.at(trianIdx));
    return ret;
}

bool RenderingTask::isObstructed(const Ray &r, const glm::vec3 &point) const {
    glm::vec3 tVec = (point - r.o) / r.d;
    float target = std::max({tVec.x, tVec.y, tVec.z});
    return kdTree->isObstructed(Ray(r), target, triangles, vertices);
}

void RenderingTask::renderBatch(std::vector<std::vector<glm::vec3>> &pixels,
                                std::queue<unsigned int> &&flatCoordsQueue,
                                CacheAlignedCounter &progress,
                                std::chrono::steady_clock::time_point &ts,
                                std::mutex &tsLock) const {
#ifdef DEBUG
    std::mt19937 randEng(42);
#else
    std::mt19937 randEng((std::random_device())());
#endif // DEBUG
    CosineSampler sampler;

    while (!flatCoordsQueue.empty()) {
        const auto p = flatCoordsQueue.front();
        unsigned int px = p % width, py = p / width;
        flatCoordsQueue.pop();
        glm::vec3 pixel(0);
        for (unsigned int i = 0; i < nSamples; i++) {
            pixel += traceRay(getPrimaryRay(px, py), recLvl, randEng, cookTorrance, sampler);
        }
        pixels.at(py).at(px) = pixel / float(nSamples);
        progress.counter++;
    }

    auto end = std::chrono::steady_clock::now();
    tsLock.lock();
    if (end > ts)
        ts = end;
    tsLock.unlock();
}

void RenderingTask::recomputeCameraParams() {
    // make up perpendicular to front
    front = glm::normalize(lookAt - viewPoint);
    up = glm::normalize(up);
    right = glm::normalize(glm::cross(front, up));
    up = glm::normalize(glm::cross(right, front)) * yView / 2.f;
    right *= (float)width * yView / (float)height / 2.f;
}

unsigned int RenderingTask::getLightIdxFromRndVal(const float rnd) const {
    return getLightIdxFromRndVal(rnd, 0, lightIndices.size() - 1);
}

unsigned int RenderingTask::getLightIdxFromRndVal(const float rnd, const unsigned int begin,
                                                  const unsigned int end) const {
    if (begin > end)
        throw std::invalid_argument("Empty search interval.");
    if (begin == end)
        return lightIndices.at(begin);
    const unsigned int mid = (begin + end) / 2;
    if (rnd <= lightPowersCdf.at(mid))
        return getLightIdxFromRndVal(rnd, begin, mid);
    else
        return getLightIdxFromRndVal(rnd, mid + 1, end);
}

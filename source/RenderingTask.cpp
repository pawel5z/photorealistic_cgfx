#include <OpenEXR/ImfChannelList.h>
#include <OpenEXR/ImfFrameBuffer.h>
#include <OpenEXR/ImfHeader.h>
#include <OpenEXR/ImfOutputFile.h>
#include <assimp/Importer.hpp>  // C++ importer interface
#include <assimp/postprocess.h> // Post processing flags
#include <assimp/scene.h>       // Output data structure
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <functional>
#include <glm/gtx/intersect.hpp>
#include <glm/gtx/norm.hpp>
#include <iostream>
#include <numeric>

#include "RenderingTask.hpp"

#include "BRDFs.hpp"
#include "ogl_interface/Axes.hpp"
#include "utils.hpp"

using namespace std::chrono_literals;
namespace fs = std::filesystem;

CacheAlignedCounter::CacheAlignedCounter(unsigned int counter) : counter(counter) {}

RenderingTask::RenderingTask(std::string rtcPath, unsigned int nSamples, unsigned int concThreads)
    : rtcPath(rtcPath), nSamples(nSamples), russianRouletteAlpha(1.f) {
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
            vertices.push_back({{v.x, v.y, v.z}, glm::normalize(glm::vec3(n.x, n.y, n.z))});
        }
    }

    std::cerr << triangles.size() << " triangles\n";
}

void RenderingTask::render() const {
    std::vector<std::vector<glm::vec3>> pixels(height, std::vector<glm::vec3>(width, glm::vec3(0)));
    std::vector<CacheAlignedCounter> progress(concThreads);
    std::vector<std::thread> ts;
    unsigned int minBatchSize = width * height / concThreads;
    auto begin = std::chrono::steady_clock::now();
    auto end = begin;
    std::mutex endLock;
    for (unsigned int i = 0; i < concThreads; i++)
        ts.emplace_back(&RenderingTask::renderBatch, this, std::ref(pixels), i * minBatchSize,
                        i != concThreads - 1 ? minBatchSize : width * height - i * minBatchSize,
                        std::ref(progress.at(i)), std::ref(end), std::ref(endLock));
    std::cout << "Rendering using " << concThreads << " thread" << (concThreads == 1 ? "" : "s")
              << "...\n";
    std::this_thread::yield();
    while (true) {
        unsigned int progressSoFar = 0;
        for (const auto &counter : progress)
            progressSoFar += counter.counter;
        std::cerr
            << "\r" << progressSoFar * 100 / (width * height)
            << "%                                                                                ";
        if (progressSoFar == width * height)
            break;
        std::this_thread::sleep_for(1s);
    }
    std::cout << "\n";
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

glm::vec3 RenderingTask::traceRay(
    const Ray &r, unsigned int maxDepth, std::mt19937 &randEng,
    const std::function<glm::vec3(const glm::vec3 &, const glm::vec3 &, const Material &)> &brdf,
    HemisphereSampler &sampler) const {
    float t;
    glm::vec3 n;
    const Material *mat;
    if (maxDepth == 0 || !findNearestIntersection(r, t, n, &mat))
        return {0, 0, 0};

    glm::vec3 color = mat->ke;
    auto [s, prob] = sampler();
    if (prob == 0.f)
        return color;

    Ray incoming(r.o + t * r.d, sampler.makeSampleRelativeToNormal(s, n));
    glm::vec3 outgoingRelToUp = glm::rotate(glm::rotation(n, glm::vec3(0, 1, 0)), -r.d);
    static thread_local std::uniform_real_distribution<float> russianRouletteDist;
    if (russianRouletteDist(randEng) <= russianRouletteAlpha)
        color += brdf(s, outgoingRelToUp, *mat) *
                 traceRay(incoming, maxDepth - 1, randEng, brdf, sampler) *
                 glm::abs(glm::cos(glm::dot(n, incoming.d))) / (prob * russianRouletteAlpha);

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

bool RenderingTask::isObstructed(const Ray &r, const Light &l) const {
    glm::vec3 tLightVec = (l.pos - r.o) / r.d;
    float tLight = std::max({tLightVec.x, tLightVec.y, tLightVec.z});
    return kdTree->isObstructed(Ray(r), l, tLight, triangles, vertices);
}

void RenderingTask::renderBatch(std::vector<std::vector<glm::vec3>> &pixels,
                                const unsigned int from, const unsigned int count,
                                CacheAlignedCounter &progress,
                                std::chrono::steady_clock::time_point &ts,
                                std::mutex &tsLock) const {
#ifdef DEBUG
    std::mt19937 randEng(42);
#else
    std::mt19937 randEng((std::random_device())());
#endif // DEBUG
    CosineSampler sampler;
    for (unsigned int p = from; p < from + count; p++) {
        unsigned int px = p % width, py = p / width;
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

#include <IL/devil_cpp_wrapper.hpp>
#include <assimp/Importer.hpp>  // C++ importer interface
#include <assimp/postprocess.h> // Post processing flags
#include <assimp/scene.h>       // Output data structure
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <glm/gtx/intersect.hpp>
#include <glm/gtx/norm.hpp>
#include <iostream>
#include <numeric>
#include <thread>

#include "RenderingTask.hpp"
#include "ogl_interface/Axes.hpp"
#include "ogl_interface/Camera.hpp"
#include "utils.hpp"

namespace fs = std::filesystem;

RenderingTask::RenderingTask(std::string rtcPath, unsigned int concThreads) : rtcPath(rtcPath) {
    this->concThreads = std::max(1U, std::min(std::thread::hardware_concurrency(), concThreads));

    std::cerr << "Reading data...\n";

    std::ifstream configFile(rtcPath);
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

    while (!configFile.eof()) {
        try {
            std::getline(configFile, line);
            lights.emplace_back(line);
        } catch (std::exception &e) {
            std::cerr << e.what() << '\n';
        }
    }

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
    std::vector<unsigned char> imgData(width * height * 3);
    std::vector<unsigned int> progress(concThreads);
    std::vector<std::thread> ts;
    unsigned int minBatchSize = width * height / concThreads;
    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
    for (unsigned int i = 0; i < concThreads; i++) {
        ts.emplace_back(&RenderingTask::renderBatch, this, std::ref(imgData), i * minBatchSize,
                        i != concThreads - 1 ? minBatchSize : width * height - i * minBatchSize,
                        std::ref(progress.at(i)));
    }
    std::cout << "Rendering using " << concThreads << " thread" << (concThreads == 1 ? "" : "s")
              << "...\n";
    while (true) {
        unsigned int progressSoFar = std::accumulate(progress.begin(), progress.end(), 0U);
        std::cerr
            << "\r" << progressSoFar * 100 / (width * height)
            << "%                                                                                ";
        if (progressSoFar == width * height)
            break;
        std::this_thread::yield();
    }
    std::cout << "\n";
    for (auto &t : ts)
        t.join();
    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();

    unsigned int tracedRaysCnt = (recLvl * (1 + lights.size()) + 1) * width * height;
    float tracingTime =
        std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count() / 1000000.f;
    std::cout << "Traced " << tracedRaysCnt << " rays in " << tracingTime << " seconds.\n"
              << tracedRaysCnt / tracingTime << " rays per second\n";

    ilEnable(IL_FILE_OVERWRITE);
    ilImage img;
    if (!img.TexImage(width, height, 1, 3, IL_RGB, IL_UNSIGNED_BYTE, imgData.data())) {
        ilLogErrorStack();
        throw std::logic_error("Error constructing image.");
    }
    if (!img.Save(outputPath.c_str(), IL_PNG)) {
        ilLogErrorStack();
        throw std::logic_error("Error saving image.");
    }
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
    for (const auto &light : rt->lights) {
        std::vector<unsigned char> byteCol = light.getByteColor();
        os << "\nL " << light.pos.x << ' ' << light.pos.y << ' ' << light.pos.z << ' '
           << (int)byteCol.at(0) << ' ' << (int)byteCol.at(1) << ' ' << (int)byteCol.at(2) << ' '
           << light.intensity;
    }
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

Ray RenderingTask::getPrimaryRay(float px, float py) const {
    return {viewPoint, glm::normalize(front + up * -(py * 2.f / (float)(height - 1) - 1.f) +
                                      right * (px * 2.f / (float)(width - 1) - 1.f))};
}

glm::vec3 RenderingTask::traceRay(const Ray &r, unsigned int maxDepth) const {
    float t;
    glm::vec3 n;
    const Material *mat;
    if (!findNearestIntersection(r, t, n, &mat))
        return {0, 0, 0};
    if (maxDepth == 0 || lights.size() == 0)
        return mat->kd;
    glm::vec3 color = mat->ka * .1f;
    glm::vec3 hit = r.o + t * r.d;
    for (auto &light : lights) {
        Ray shadowRay = {hit, glm::normalize(light.pos - hit)};
        float sqDist = glm::distance2(hit, light.pos);
        if (isObstructed(shadowRay, light))
            continue;
        float dTerm = glm::dot(n, shadowRay.d);
        color +=
            (mat->kd * glm::max(0.f, dTerm) +
             mat->ks * (float)(dTerm > 0.f) *
                 glm::max(0.f, glm::pow(glm::dot(-r.d, glm::reflect(-shadowRay.d, n)), mat->ns))) *
            light.color * light.intensity / sqDist;
    }
    if (mat->ks.r != 0.f || mat->ks.g != 0.f || mat->ks.b != 0.f)
        color += mat->ks * traceRay({hit, glm::reflect(r.d, n)}, maxDepth - 1);
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

void RenderingTask::renderBatch(std::vector<unsigned char> &imgData, const unsigned int from,
                                const unsigned int count, unsigned int &progress) const {
    for (unsigned int p = from; p < from + count; p++) {
        float px = p % width, py = (height - 1 - (p / width));
        glm::vec3 col(0.f);
        for (const glm::vec2 off : std::vector<glm::vec2>(4, glm::vec2(.25f)))
            col += glm::clamp(traceRay(getPrimaryRay(px - off.x, py - off.y), recLvl), 0.f, 1.f) *
                   255.f;
        col /= 4.f;
        for (glm::length_t i = 0; i < col.length(); i++)
            imgData.at(3 * p + i) = col[i];
        progress++;
    }
}

void RenderingTask::recomputeCameraParams() {
    // make up perpendicular to front
    front = glm::normalize(lookAt - viewPoint);
    up = glm::normalize(up);
    right = glm::normalize(glm::cross(front, up));
    up = glm::normalize(glm::cross(right, front)) * yView / 2.f;
    right *= (float)width * yView / (float)height / 2.f;
}

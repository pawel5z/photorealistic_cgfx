#include <IL/devil_cpp_wrapper.hpp>
#include <assimp/Importer.hpp>  // C++ importer interface
#include <assimp/postprocess.h> // Post processing flags
#include <assimp/scene.h>       // Output data structure
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
                                                          aiProcess_FixInfacingNormals);
    if (scene == nullptr) {
        std::cerr << importer.GetErrorString() << '\n';
        throw std::logic_error("Error while importing \"" + objPath + "\".\n");
    }

    for (int i = 0; i < scene->mNumMaterials; i++)
        mats.emplace_back(scene->mMaterials[i]);

    for (int i = 0; i < scene->mNumMeshes; i++)
        meshes.emplace_back(scene->mMeshes[i], mats);
}

void RenderingTask::render() const {
    std::vector<unsigned char> imgData(width * height * 3);
    std::vector<unsigned int> progress(concThreads);
    std::vector<std::thread> ts;
    unsigned int minBatchSize = width * height / concThreads;
    for (unsigned int i = 0; i < concThreads; i++) {
        ts.emplace_back(&RenderingTask::renderBatch, this, std::ref(imgData), i * minBatchSize,
                        i != concThreads - 1 ? minBatchSize : width * height - i * minBatchSize,
                        std::ref(progress[i]));
    }
    std::cout << "Rendering using " << concThreads << " threads.\n";
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

void RenderingTask::updateRTCFile() {
    std::ofstream fout(rtcPath);
    fout << this;
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
           << (int)byteCol[0] << ' ' << (int)byteCol[1] << ' ' << (int)byteCol[2] << ' '
           << light.intensity;
    }
    return os;
}

Ray RenderingTask::getPrimaryRay(unsigned int px, unsigned int py) const {
    return {viewPoint, glm::normalize(front + up * -((float)py * 2.f / (float)(height - 1) - 1.f) +
                                      right * ((float)px * 2.f / (float)(width - 1) - 1.f))};
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
    color += mat->ks * traceRay({hit, glm::reflect(r.d, n)}, maxDepth - 1);
    return color;
}

bool RenderingTask::findNearestIntersection(const Ray &r, float &t, glm::vec3 &n,
                                            const Material **mat) const {
    float tNearest = std::numeric_limits<float>::max();
    glm::vec2 baryPos;
    for (const auto &mesh : meshes)
        for (const auto &tri : mesh.triangles) {
            Vertex a = mesh.vertices[tri.indices[0]];
            Vertex b = mesh.vertices[tri.indices[1]];
            Vertex c = mesh.vertices[tri.indices[2]];
            if (!glm::intersectRayTriangle(r.o, r.d, a.pos, b.pos, c.pos, baryPos, t))
                continue;
            if (.001f < t && t < tNearest) {
                tNearest = t;
                n = a.norm + baryPos.x * (b.norm - a.norm) + baryPos.y * (c.norm - a.norm);
                *mat = mesh.mat;
            }
        }
    if (tNearest == std::numeric_limits<float>::max())
        return false;
    t = tNearest;
    n = glm::normalize(n);
    return true;
}

bool RenderingTask::isObstructed(const Ray &r, const Light &l) const {
    for (const auto &mesh : meshes)
        for (const auto &tri : mesh.triangles) {
            Vertex a = mesh.vertices[tri.indices[0]];
            Vertex b = mesh.vertices[tri.indices[1]];
            Vertex c = mesh.vertices[tri.indices[2]];
            glm::vec2 baryPos;
            float t;
            if (glm::intersectRayTriangle(r.o, r.d, a.pos, b.pos, c.pos, baryPos, t))
                if (t > .001f && glm::distance2(r.o, r.o + r.d * t) < glm::distance2(r.o, l.pos))
                    return true;
        }
    return false;
}

void RenderingTask::renderBatch(std::vector<unsigned char> &imgData, const unsigned int from,
                                const unsigned int count, unsigned int &progress) const {
    for (unsigned int p = from; p < from + count; p++) {
        glm::vec3 col =
            glm::clamp(traceRay(getPrimaryRay(p % width, (height - 1 - (p / width))), recLvl), 0.f,
                       1.f) *
            255.f;
        for (glm::length_t i = 0; i < col.length(); i++)
            imgData[3 * p + i] = col[i];
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

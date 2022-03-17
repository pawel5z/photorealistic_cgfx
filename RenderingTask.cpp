#include <fstream>
#include <cstdio>
#include <iostream>
#include <assimp/Importer.hpp> // C++ importer interface
#include <assimp/scene.h> // Output data structure
#include <assimp/postprocess.h> // Post processing flags
#include <filesystem>
#include <IL/devil_cpp_wrapper.hpp>
#include <glm/gtx/intersect.hpp>

#include "RenderingTask.hpp"
#include "utils.hpp"

namespace fs = std::filesystem;

RenderingTask::RenderingTask(std::string rtcPath) {
    std::ifstream configFile(rtcPath);
    fs::path rtcDir = fs::path(rtcPath).parent_path();
    std::string line;

    std::getline(configFile, line); // ignore comment line

    std::string objPath;
    std::getline(configFile, objPath);
    objPath = fs::path(rtcDir).append(objPath).string();

    std::getline(configFile, outputPath);
    outputPath = fs::path(rtcDir).append(outputPath).string();

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
        std::cerr << "Could not parse Vector Up.\n";
        exit(EXIT_FAILURE);
    }
    // make up perpendicular to front
    front = glm::normalize(lookAt - viewPoint);
    up = glm::normalize(up);
    right = glm::normalize(glm::cross(front, up));
    up = glm::normalize(glm::cross(right, front)) * yView / 2.f;
    right *= (float)width * yView / (float)height / 2.f;

    std::getline(configFile, line);
    try {
        yView = std::stof(line);
    } catch(const std::exception &e) {
        std::cerr << e.what() << '\n'
            << "Could not parse yview. Using default 1.0.\n";
    }

    while (!configFile.eof()) {
        try {
            std::getline(configFile, line);
            lights.emplace_back(line);
        }
        catch (std::exception &e) {
            std::cerr << e.what() << '\n';
        }
    }

    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(objPath, aiProcess_Triangulate);
    if (scene == nullptr) {
        std::cerr << importer.GetErrorString() << '\n';
        throw std::logic_error("Error while importing \"" + objPath + "\".\n");
    }

    for (int i = 0; i < scene->mNumMaterials; i++)
        mats.emplace_back(scene->mMaterials[i]);

    for (int i = 0; i < scene->mNumMeshes; i++)
        meshes.emplace_back(scene->mMeshes[i], mats);
}

void RenderingTask::render() {
    std::vector<unsigned char> imgData;
    for (unsigned int py = 0; py < height; py++)
        for (unsigned int px = 0; px < width; px++) {
            glm::vec3 col = glm::clamp(glm::clamp(traceRay(getPrimaryRay(px, (height - 1 - py)), recLvl), 0.f, 1.f) * 255.f, 0.f, 255.f);
            for (glm::length_t i = 0; i < col.length(); i++)
                imgData.push_back(col[i]);
        }
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

Ray RenderingTask::getPrimaryRay(unsigned int px, unsigned int py) {
    Ray r = {viewPoint};
    r.d = front + up * -((float)py * 2.f / (float)(height - 1) - 1.f) + right * ((float)px * 2.f / (float)(width - 1) - 1.f);
    return r;
}

glm::vec3 RenderingTask::traceRay(const Ray &r, unsigned int maxDepth) {
    float t;
    glm::vec3 n;
    const Material *mat;
    if (!findNearestIntersection(r, t, n, &mat))
        return {0, 0, 0};
    if (maxDepth == 0 || lights.size() == 0)
        return mat->kd;
    // TODO recursive call, shadow rays, reflected rays
}

bool RenderingTask::findNearestIntersection(const Ray &r, float &t, glm::vec3 &n, const Material **mat) {
    float tNearest = std::numeric_limits<float>().max();
    glm::vec2 baryPos;
    for (const auto &mesh : meshes)
        for (const auto &tri : mesh.triangles) {
            Vertex a = mesh.vertices[tri.indices[0]];
            Vertex b = mesh.vertices[tri.indices[1]];
            Vertex c = mesh.vertices[tri.indices[2]];
            if (!glm::intersectRayTriangle(r.o, r.d, a.pos, b.pos, c.pos, baryPos, t))
                continue;
            if (t < tNearest) {
                tNearest = t;
                n = a.norm + baryPos.x * (b.norm - a.norm) + baryPos.y * (c.norm - a.norm);
                *mat = mesh.mat;
            }
        }
    if (tNearest == std::numeric_limits<float>().max())
        return false;
    t = tNearest;
    return true;
}

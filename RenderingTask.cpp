#include <fstream>
#include <cstdio>
#include <iostream>
#include <assimp/Importer.hpp> // C++ importer interface
#include <assimp/scene.h> // Output data structure
#include <assimp/postprocess.h> // Post processing flags
#include <filesystem>
#include <IL/devil_cpp_wrapper.hpp>

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
    lookAt = glm::normalize(lookAt);

    std::getline(configFile, line);
    if (sscanf(line.c_str(), "%f %f %f", &up.x, &up.y, &up.z) < 3) {
        std::cerr << "Could not parse Vector Up.\n";
        exit(EXIT_FAILURE);
    }
    up = glm::normalize(up);

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
            glm::vec3 col = traceRay(getPrimaryRay(px, py), recLvl);
            imgData.push_back(col.r * (unsigned char)(~0));
            imgData.push_back(col.g * (unsigned char)(~0));
            imgData.push_back(col.b * (unsigned char)(~0));
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
    glm::vec3 front = glm::normalize(lookAt - viewPoint);
    glm::vec3 right = glm::normalize(glm::cross(front, up)) * (float)width * yView / (float)height / 2.f;
    r.d = front + up * -((float)py * 2.f / (float)(height - 1) - 1.f) + right * ((float)px * 2.f / (float)(width - 1) - 1.f);
    return r;
}

glm::vec3 RenderingTask::traceRay(Ray r, unsigned int maxDepth) {
    // TODO actual ray tracing
    return {0, 0, .5f};
}

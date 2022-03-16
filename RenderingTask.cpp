#include <fstream>
#include <cstdio>
#include <iostream>
#include <assimp/Importer.hpp> // C++ importer interface
#include <assimp/scene.h> // Output data structure
#include <assimp/postprocess.h> // Post processing flags
#include <filesystem>

#include "RenderingTask.hpp"

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

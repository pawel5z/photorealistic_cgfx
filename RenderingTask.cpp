#include "ogl_interface/Axes.hpp"
#include "ogl_interface/Camera.hpp"
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
#include <thread>

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
        std::cerr << "Could not parse Vector Up. Using default (0, 1, 0).\n";
    }

    std::getline(configFile, line);
    try {
        yView = std::stof(line);
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n'
                  << "Could not parse yview. Using default 1.0.\n";
    }

    // make up perpendicular to front
    front = glm::normalize(lookAt - viewPoint);
    up = glm::normalize(up);
    right = glm::normalize(glm::cross(front, up));
    up = glm::normalize(glm::cross(right, front)) * yView / 2.f;
    right *= (float)width * yView / (float)height / 2.f;

    while (!configFile.eof()) {
        try {
            std::getline(configFile, line);
            lights.emplace_back(line);
        } catch (std::exception &e) {
            std::cerr << e.what() << '\n';
        }
    }

    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(objPath, aiProcess_Triangulate | aiProcess_GenNormals | aiProcess_FixInfacingNormals);
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
    auto procCnt = std::thread::hardware_concurrency();
    if (std::thread::hardware_concurrency() <= 1) {
        renderBatch(imgData, 0, width * height);
    } else {
        std::vector<std::thread> ts;
        for (unsigned int i = 0; i < procCnt; i++) {
            unsigned int minBatchSize = width * height / procCnt;
            ts.emplace_back(&RenderingTask::renderBatch, this, std::ref(imgData), i * minBatchSize, i != procCnt - 1 ? minBatchSize : width * height - i * minBatchSize);
        }
        for (auto &t : ts)
            t.join();
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

void RenderingTask::preview() {
    RTWindow window;
    window.Init(1280, 720, "Preview");
    window.MainLoop(this);
}

Ray RenderingTask::getPrimaryRay(unsigned int px, unsigned int py) const {
    Ray r = {viewPoint};
    r.d = glm::normalize(front + up * -((float)py * 2.f / (float)(height - 1) - 1.f) + right * ((float)px * 2.f / (float)(width - 1) - 1.f));
    return r;
}

glm::vec3 RenderingTask::traceRay(const Ray &r, unsigned int maxDepth) const {
    float t;
    glm::vec3 n;
    const Material *mat;
    if (!findNearestIntersection(r, t, n, &mat))
        return {0, 0, 0};
    if (maxDepth == 0 || lights.size() == 0)
        return mat->kd;
    glm::vec3 color = mat->ka;
    glm::vec3 hit = r.o + t * r.d;
    glm::vec3 viewer = -r.d;
    for (auto &light : lights) {
        Ray shadowRay = {hit, glm::normalize(light.pos - hit)};
        float sqDist = glm::distance2(hit, light.pos);
        if (isObstructed(shadowRay))
            continue;
        float dTerm = glm::dot(n, shadowRay.d);
        color += (mat->kd * glm::max(0.f, dTerm) +
                  mat->ks * (dTerm > 0.f ? glm::max(0.f, glm::pow(glm::dot(viewer, glm::reflect(shadowRay.d, n)), mat->ns)) : 0.f)) *
                 light.color * light.intensity / (4.f * glm::pi<float>() * sqDist);
    }
    color += mat->ks * traceRay({r.o + t * r.d, glm::reflect(viewer, n)}, maxDepth - 1);
    return color;
}

bool RenderingTask::findNearestIntersection(const Ray &r, float &t, glm::vec3 &n, const Material **mat) const {
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

bool RenderingTask::isObstructed(const Ray &r) const {
    for (const auto &mesh : meshes)
        for (const auto &tri : mesh.triangles) {
            Vertex a = mesh.vertices[tri.indices[0]];
            Vertex b = mesh.vertices[tri.indices[1]];
            Vertex c = mesh.vertices[tri.indices[2]];
            glm::vec2 baryPos;
            float t;
            if (glm::intersectRayTriangle(r.o, r.d, a.pos, b.pos, c.pos, baryPos, t))
                if (t > .001f)
                    return true;
        }
    return false;
}

void RenderingTask::renderBatch(std::vector<unsigned char> &imgData, const unsigned int from, const unsigned int count) const {
    for (unsigned int p = from; p < from + count; p++) {
        glm::vec3 col = glm::clamp(glm::clamp(traceRay(getPrimaryRay(p % width, (height - 1 - (p / width))), recLvl), 0.f, 1.f) * 255.f, 0.f, 255.f);
        for (glm::length_t i = 0; i < col.length(); i++)
            imgData[3 * p + i] = col[i];
    }
}

void RenderingTask::RTWindow::MainLoop(RenderingTask *rt) {
    ViewportOne(0, 0, wd, ht);
    Camera camera(glm::degrees(glm::atan(rt->yView / 2.f)), aspect, .1f, 10000.f);
    camera.pos = rt->viewPoint;
    camera.rot = glm::quatLookAtLH(glm::normalize(rt->lookAt - rt->viewPoint), rt->up);
    float spd = .1f;
    float angSpd = .001f;

    Axes axes("axes.vert", "axes.frag");

    GLuint meshPId = 0;
    compileProgramFromFile(meshPId, "mesh.vert", "mesh.frag");
    GLuint arrays[rt->meshes.size()];
    glGenVertexArrays(rt->meshes.size(), arrays);
    GLuint buffers[rt->meshes.size()];
    glGenBuffers(rt->meshes.size(), buffers);
    GLuint elementBuffers[rt->meshes.size()];
    glGenBuffers(rt->meshes.size(), elementBuffers);
    for (unsigned int i = 0; i < rt->meshes.size(); i++) {
        auto &mesh = rt->meshes[i];
        glBindVertexArray(arrays[i]);
        glBindBuffer(GL_ARRAY_BUFFER, buffers[i]);
        glBufferData(GL_ARRAY_BUFFER, mesh.vertices.size() * sizeof(Vertex), mesh.vertices.data(), GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementBuffers[i]);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh.triangles.size() * sizeof(Triangle), mesh.triangles.data(), GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, false, 2 * sizeof(glm::vec3), nullptr);
    }

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    double refMouseX = 0, refMouseY = 0;
    while (glfwGetKey(win(), GLFW_KEY_ESCAPE) != GLFW_PRESS &&
           glfwWindowShouldClose(win()) == 0) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        AGLErrors("before drawing");
        // >>> render
        axes.draw(camera);
        glUseProgram(meshPId);
        glUniformMatrix4fv(0, 1, false, &camera.getPVMat()[0][0]);
        for (unsigned int i = 0; i < rt->meshes.size(); i++) {
            glUniform3fv(1, 1, &rt->mats[i].kd[0]);
            glBindVertexArray(arrays[i]);
            glDrawElements(GL_TRIANGLES, rt->meshes[i].triangles.size() * 3, GL_UNSIGNED_INT, nullptr);
        }
        // <<< render
        AGLErrors("after drawing");
        glfwSwapBuffers(win());
        glfwPollEvents();
        // >>> process events
        camera.setAspect(aspect);
        if (glfwGetKey(win(), GLFW_KEY_W) == GLFW_PRESS)
            camera.pos += camera.forward() * spd;
        if (glfwGetKey(win(), GLFW_KEY_S) == GLFW_PRESS)
            camera.pos -= camera.forward() * spd;
        if (glfwGetKey(win(), GLFW_KEY_D) == GLFW_PRESS)
            camera.pos += camera.right() * spd;
        if (glfwGetKey(win(), GLFW_KEY_A) == GLFW_PRESS)
            camera.pos -= camera.right() * spd;
        if (glfwGetKey(win(), GLFW_KEY_R) == GLFW_PRESS)
            camera.pos += camera.up() * spd;
        if (glfwGetKey(win(), GLFW_KEY_F) == GLFW_PRESS)
            camera.pos -= camera.up() * spd;
        if (glfwGetKey(win(), GLFW_KEY_E) == GLFW_PRESS)
            camera.rotate(Transform::FORWARD, angSpd * 5.f);
        if (glfwGetKey(win(), GLFW_KEY_Q) == GLFW_PRESS)
            camera.rotate(Transform::FORWARD, -angSpd * 5.f);
        if (glfwGetMouseButton(win(), GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
            double xPos, yPos;
            glfwGetCursorPos(win(), &xPos, &yPos);
            camera.rotate(Transform::UP, (refMouseX - xPos) * angSpd);
            camera.rotate(Transform::RIGHT, (refMouseY - yPos) * angSpd);
        }
        if (glfwGetKey(win(), GLFW_KEY_BACKSPACE) == GLFW_PRESS) {
            camera.pos = rt->viewPoint;
            camera.rot = glm::quatLookAtLH(glm::normalize(rt->lookAt - rt->viewPoint), rt->up);
        }
        glfwGetCursorPos(win(), &refMouseX, &refMouseY);
        // <<< process events
        WaitForFixedFPS();
    }
    glDeleteProgram(meshPId);
    glDeleteBuffers(rt->meshes.size(), buffers);
    glDeleteBuffers(rt->meshes.size(), elementBuffers);
    glDeleteVertexArrays(rt->meshes.size(), arrays);
}

void RenderingTask::RTWindow::KeyCB(int key, int scancode, int action, int mods) {}

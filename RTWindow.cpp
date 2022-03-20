#include <iostream>

#include "RenderingTask.hpp"
#include "ogl_interface/Axes.hpp"
#include "utils.hpp"

RenderingTask::RTWindow::RTWindow(RenderingTask *rt) : rt(rt) {}

void RenderingTask::RTWindow::MainLoop() {
    ViewportOne(0, 0, wd, ht);
    Camera camera(glm::degrees(glm::atan(rt->yView / 2.f)), aspect, .1f, 10000.f);
    camera.pos = rt->viewPoint;
    camera.rot = glm::quatLookAtLH(glm::normalize(rt->lookAt - rt->viewPoint), rt->up);
    float spd = 1.f, spdMult = 1.f, angSpd = .001f;

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
        glBufferData(GL_ARRAY_BUFFER, mesh.vertices.size() * sizeof(Vertex), mesh.vertices.data(),
                     GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementBuffers[i]);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh.triangles.size() * sizeof(Triangle),
                     mesh.triangles.data(), GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, false, 2 * sizeof(glm::vec3), nullptr);
    }

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    double refMouseX = 0, refMouseY = 0;
    while (glfwGetKey(win(), GLFW_KEY_ESCAPE) != GLFW_PRESS && glfwWindowShouldClose(win()) == 0) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        AGLErrors("before drawing");
        // >>> render
        axes.draw(camera);
        glUseProgram(meshPId);
        glUniformMatrix4fv(0, 1, false, &camera.getPVMat()[0][0]);
        for (unsigned int i = 0; i < rt->meshes.size(); i++) {
            glUniform3fv(1, 1, &rt->meshes[i].mat->kd[0]);
            glBindVertexArray(arrays[i]);
            glDrawElements(GL_TRIANGLES, rt->meshes[i].triangles.size() * 3, GL_UNSIGNED_INT,
                           nullptr);
        }
        // <<< render
        AGLErrors("after drawing");
        glfwSwapBuffers(win());
        glfwPollEvents();
        // >>> process events
        camera.setAspect(aspect);
        if (glfwGetKey(win(), GLFW_KEY_W) == GLFW_PRESS)
            camera.pos += camera.forward() * spd * spdMult;
        if (glfwGetKey(win(), GLFW_KEY_S) == GLFW_PRESS)
            camera.pos -= camera.forward() * spd * spdMult;
        if (glfwGetKey(win(), GLFW_KEY_D) == GLFW_PRESS)
            camera.pos += camera.right() * spd * spdMult;
        if (glfwGetKey(win(), GLFW_KEY_A) == GLFW_PRESS)
            camera.pos -= camera.right() * spd * spdMult;
        if (glfwGetKey(win(), GLFW_KEY_R) == GLFW_PRESS)
            camera.pos += camera.up() * spd * spdMult;
        if (glfwGetKey(win(), GLFW_KEY_F) == GLFW_PRESS)
            camera.pos -= camera.up() * spd * spdMult;
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
        if (glfwGetKey(win(), GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
            if (std::numeric_limits<float>::max() / 2.f >= spdMult)
                spdMult *= 2.f;
        if (glfwGetKey(win(), GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
            if (std::numeric_limits<float>::epsilon() * 2.f <= spdMult)
                spdMult /= 2.f;
        if (glfwGetKey(win(), GLFW_KEY_SPACE)) {
            updateRTCamera(camera);
            rt->renderPreview = true;
            break;
        }
        if (glfwGetKey(win(), GLFW_KEY_ENTER)) {
            updateRTCamera(camera);
            rt->updateRTCFile();
            std::cout << "Saved changes to \"" << rt->rtcPath << "\".\n";
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

void RenderingTask::RTWindow::updateRTCamera(const Camera &camera) {
    rt->viewPoint = camera.pos;
    rt->lookAt = camera.pos + camera.forward();
    rt->up = camera.up();
    rt->recomputeCameraParams();
}

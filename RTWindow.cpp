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

    GLuint buffer;
    glGenBuffers(1, &buffer);
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    glBufferData(GL_ARRAY_BUFFER, rt->vertices.size() * sizeof(Vertex), rt->vertices.data(),
                 GL_STATIC_DRAW);

    GLuint elementBuffers[rt->meshes.size()];
    glGenBuffers(rt->meshes.size(), elementBuffers);

    for (unsigned int i = 0; i < rt->meshes.size(); i++) {
        auto &mesh = rt->meshes.at(i);
        glBindVertexArray(arrays[i]);
        glBindBuffer(GL_ARRAY_BUFFER, buffer);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementBuffers[i]);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh.trianglesCnt * sizeof(Triangle),
                     &rt->triangles.at(mesh.firstTriangleIdx), GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(Vertex), nullptr);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, false, sizeof(Vertex), (void *)sizeof(glm::vec3));
    }

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    double refMouseX = 0, refMouseY = 0;
    int lShift = GLFW_RELEASE, lCtrl = GLFW_RELEASE, enter = GLFW_RELEASE;
    while (glfwGetKey(win(), GLFW_KEY_ESCAPE) != GLFW_PRESS && glfwWindowShouldClose(win()) == 0) {
        std::cerr
            << "\rCamera position: " << camera.pos.x << ' ' << camera.pos.y << ' ' << camera.pos.z
            << "                                                                                ";
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        AGLErrors("before drawing");
        // >>> render
        axes.draw(camera);
        glUseProgram(meshPId);
        glUniformMatrix4fv(0, 1, false, &camera.getPVMat()[0][0]);
        for (unsigned int i = 0; i < rt->meshes.size(); i++) {
            glUniform3fv(1, 1, &rt->mats.at(rt->meshes.at(i).matIdx).kd[0]);
            glBindVertexArray(arrays[i]);
            glDrawElements(GL_TRIANGLES, rt->meshes.at(i).trianglesCnt * 3, GL_UNSIGNED_INT,
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
        if (glfwGetKey(win(), GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
            if (lShift == GLFW_RELEASE)
                if (std::numeric_limits<float>::max() / 2.f >= spdMult)
                    spdMult *= 2.f;
            lShift = GLFW_PRESS;
        } else
            lShift = GLFW_RELEASE;
        if (glfwGetKey(win(), GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) {
            if (lCtrl == GLFW_RELEASE)
                if (std::numeric_limits<float>::epsilon() * 2.f <= spdMult)
                    spdMult /= 2.f;
            lCtrl = GLFW_PRESS;
        } else
            lCtrl = GLFW_RELEASE;
        if (glfwGetKey(win(), GLFW_KEY_SPACE) == GLFW_PRESS) {
            updateRTCamera(camera);
            rt->renderPreview = true;
            break;
        }
        if (glfwGetKey(win(), GLFW_KEY_ENTER) == GLFW_PRESS) {
            if (enter == GLFW_RELEASE) {
                updateRTCamera(camera);
                rt->updateRTCFile();
                std::cout << "Saved changes to \"" << rt->rtcPath << "\".\n";
            }
            enter = GLFW_PRESS;
        } else
            enter = GLFW_RELEASE;
        glfwGetCursorPos(win(), &refMouseX, &refMouseY);
        // <<< process events
        WaitForFixedFPS();
    }
    std::cerr << "\n";
    glDeleteProgram(meshPId);
    glDeleteBuffers(rt->meshes.size(), &buffer);
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

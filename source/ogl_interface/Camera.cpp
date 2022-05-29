#include "Camera.hpp"

#include <glm/gtx/vector_angle.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
#include <cstdio>
#include <stdexcept>

Camera::Camera(float fovY, float aspect, float near, float far) : fovY(fovY), aspect(aspect), nf(near, far) {}

Camera::Camera(float left, float right, float bottom, float top, float near, float far) : lr(left, right),
                                                                                          bt(bottom, top),
                                                                                          nf(near, far) {
    type = ORTHOGRAPHIC;
}

glm::mat4 Camera::getProjectionMat() const {
    switch (type) {
        case PERSPECTIVE:
            return glm::perspective(glm::radians(fovY), aspect, nf[0], nf[1]);
        case ORTHOGRAPHIC:
            return glm::ortho(lr[0], lr[1], bt[0], bt[1], nf[0], nf[1]);
        default:
            throw std::logic_error("invalid camera type");
    }
}

glm::mat4 Camera::getViewMat() const {
    return glm::lookAt(pos, pos + forward(), up());
}

glm::mat4 Camera::getPVMat() const {
    return getProjectionMat() * getViewMat();
}

CameraType Camera::getType() const {
    return type;
}

void Camera::setType(CameraType type) {
    Camera::type = type;
}

glm::vec2 Camera::getNf() const {
    return nf;
}

void Camera::setNf(const glm::vec2 &nf) {
    Camera::nf = nf;
}

float Camera::getFovY() const {
    return fovY;
}

void Camera::setFovY(float fovY) {
    Camera::fovY = fovY;
}

float Camera::getAspect() const {
    return aspect;
}

void Camera::setAspect(float aspect) {
    Camera::aspect = aspect;
}

glm::vec2 Camera::getLr() const {
    return lr;
}

void Camera::setLr(const glm::vec2 &lr) {
    Camera::lr = lr;
}

glm::vec2 Camera::getBt() const {
    return bt;
}

void Camera::setBt(const glm::vec2 &bt) {
    Camera::bt = bt;
}

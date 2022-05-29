#include "Transform.hpp"

glm::mat4 Transform::getModelMat() const {
    return glm::translate(glm::mat4(1.0f), pos) * glm::toMat4(rot) * getScaleMat();
}

void Transform::setRot(const glm::vec3 &rot) {
    Transform::rot = glm::quat(rot);
}

glm::vec3 Transform::getRot() const {
    return glm::eulerAngles(rot);
}

glm::mat4 Transform::getScaleMat() const {
    return glm::scale(scale);
}

glm::mat4 Transform::getRotMat() const {
    return glm::toMat4(rot);
}

glm::vec3 Transform::forward() const {
    return getRotMat() * glm::vec4(FORWARD, 0);
}

glm::vec3 Transform::right() const {
    return getRotMat() * glm::vec4(RIGHT, 0);
}

glm::vec3 Transform::up() const {
    return getRotMat() * glm::vec4(UP, 0);
}

void Transform::rotate(glm::vec3 axis, float angle, Space space) {
    if (space == SELF)
        axis = getRotMat() * glm::vec4(axis, 0);
    rot = glm::angleAxis(angle, glm::normalize(axis)) * rot;
}

void Transform::rotateAround(glm::vec3 point, glm::vec3 axis, float angle) {
    pos -= point;
    pos = glm::toMat4(glm::angleAxis(angle, glm::normalize(axis))) * glm::vec4(pos, 1.f);
    pos += point;
}

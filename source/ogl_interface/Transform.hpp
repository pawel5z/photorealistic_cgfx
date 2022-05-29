#ifndef TRANSFORM_HPP
#define TRANSFORM_HPP

#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

enum Space {
    SELF,
    WORLD
};

class Transform {
public:
    static constexpr glm::vec3 ZERO = glm::vec3(0, 0, 0);
    static constexpr glm::vec3 ONE = glm::vec3(1, 1, 1);
    static constexpr glm::vec3 UP = glm::vec3(0, 1, 0);
    static constexpr glm::vec3 DOWN = -UP;
    static constexpr glm::vec3 FORWARD = glm::vec3(0, 0, 1);
    static constexpr glm::vec3 BACKWARD = -FORWARD;
    static constexpr glm::vec3 RIGHT = glm::vec3(-1, 0, 0);
    static constexpr glm::vec3 LEFT = -RIGHT;

    glm::vec3 pos = glm::vec3(0, 0, 0);
    glm::quat rot = glm::quat();
    glm::vec3 scale = glm::vec3(1, 1, 1);

    glm::vec3 getRot() const;
    void setRot(const glm::vec3 &rot);
    glm::vec3 forward() const;
    glm::vec3 right() const;
    glm::vec3 up() const;
    void rotate(glm::vec3 axis, float angle, Space space = SELF);
    void rotateAround(glm::vec3 point, glm::vec3 axis, float angle);

protected:
    glm::mat4 getScaleMat() const;
    glm::mat4 getRotMat() const;
    glm::mat4 getModelMat() const;
};

#endif //TRANSFORM_HPP

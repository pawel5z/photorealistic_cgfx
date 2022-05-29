#ifndef CAMERA_HPP
#define CAMERA_HPP

#include "Transform.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

enum CameraType {
    PERSPECTIVE,
    ORTHOGRAPHIC
};

class Camera : public Transform {
public:
    Camera() = default;
    Camera(float fovY, float aspect, float near, float far);
    Camera(float left, float right, float bottom, float top, float near, float far);
    glm::mat4 getProjectionMat() const;
    glm::mat4 getViewMat() const;
    glm::mat4 getPVMat() const;
    CameraType getType() const;
    void setType(CameraType type);
    glm::vec2 getNf() const;
    void setNf(const glm::vec2 &nf);
    float getFovY() const;
    void setFovY(float fovY);
    float getAspect() const;
    void setAspect(float aspect);
    glm::vec2 getLr() const;
    void setLr(const glm::vec2 &lr);
    glm::vec2 getBt() const;
    void setBt(const glm::vec2 &bt);

private:
    inline static constexpr glm::vec3 defaultDir = FORWARD;

    CameraType type = PERSPECTIVE;
    glm::vec2 nf = glm::vec2(0.1, 100);

    // PERSPECTIVE specific
    float fovY = 30;
    float aspect = 4.0f / 3.0f;

    // ORTHOGRAPHIC specific
    glm::vec2 lr = glm::vec2(-10, 10);
    glm::vec2 bt = glm::vec2(-10, 10);
};

#endif //CAMERA_HPP

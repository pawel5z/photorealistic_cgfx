#ifndef RAY_HPP
#define RAY_HPP

#include <glm/glm.hpp>

class Ray {
public:
    glm::vec3 o; // origin
    glm::vec3 d; // direction
    float tMin;
    float tMax;

    Ray(glm::vec3 o, glm::vec3 d, float tMin = 0.f, float tMax = std::numeric_limits<float>::max());
};

#endif // !RAY_HPP

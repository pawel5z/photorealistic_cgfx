#ifndef RAY_HPP
#define RAY_HPP

#include <glm/glm.hpp>

class Ray {
public:
    glm::dvec3 o; // origin
    glm::dvec3 d; // direction
    double tMin;
    double tMax;

    Ray(glm::dvec3 o, glm::dvec3 d, double tMin = 0.f,
        double tMax = std::numeric_limits<double>::max());
};

#endif // !RAY_HPP

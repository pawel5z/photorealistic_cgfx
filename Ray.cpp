#include "Ray.hpp"

Ray::Ray(glm::dvec3 o, glm::dvec3 d, double tMin, double tMax)
    : o(o), d(d), tMin(tMin), tMax(tMax) {}

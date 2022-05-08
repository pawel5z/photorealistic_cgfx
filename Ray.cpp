#include "Ray.hpp"

Ray::Ray(glm::vec3 o, glm::vec3 d, float tMin, float tMax) : o(o), d(d), tMin(tMin), tMax(tMax) {}

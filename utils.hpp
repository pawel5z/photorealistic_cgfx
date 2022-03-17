#ifndef UTILS_HPP
#define UTILS_HPP

#include <IL/il.h>
#include <IL/ilu.h>

#define EPSILON 0.000001f

void ilLogErrorStack();
bool intersect_triangle(const glm::vec3 &orig, const glm::vec3 &dir,
                        const glm::vec3 &vert0, const glm::vec3 &vert1, const glm::vec3 &vert2,
                        float *t, float *u, float *v);

#endif // UTILS_HPP

#ifndef UTILS_HPP
#define UTILS_HPP

#include <epoxy/gl.h>
#include <glm/glm.hpp>

void ilLogErrorStack();
/**
 * >>>
 * Taken from
 * AGL3:  GL/GLFW init AGLWindow and AGLDrawable class definitions
 * Ver.3  14.I.2020 (c) A. Łukaszewski
 */
int compileProgram(GLuint &pId, const char *vs, const char *fs, const char *gs = nullptr);
int compileProgramFromFile(GLuint &pId, const char *vs, const char *fs, const char *gs = nullptr);
/**
 * <<<
 * Taken from
 * AGL3:  GL/GLFW init AGLWindow and AGLDrawable class definitions
 * Ver.3  14.I.2020 (c) A. Łukaszewski
 */

// >>> adapted from https://ii.uni.wroc.pl/~anl/dyd/RGK/files/triangle_moller.c

bool intersectRayTriangle(const glm::vec3 &orig, const glm::vec3 &dir, const glm::vec3 &vert0,
                          const glm::vec3 &vert1, const glm::vec3 &vert2, glm::vec2 &baryPosition,
                          float &t);
// <<< adapted from https://ii.uni.wroc.pl/~anl/dyd/RGK/files/triangle_moller.c

#endif // !UTILS_HPP

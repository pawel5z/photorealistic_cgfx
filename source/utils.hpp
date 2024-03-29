#pragma once

#include <epoxy/gl.h> // must be included before glm
#include <glm/glm.hpp>

#include <iostream>

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

std::ostream &operator<<(std::ostream &os, const glm::vec3 &v);

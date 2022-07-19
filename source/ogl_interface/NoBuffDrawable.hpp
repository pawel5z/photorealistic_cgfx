#pragma once

#include "Camera.hpp"

#include <epoxy/gl.h>
#include <epoxy/glx.h>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

#include <fstream>
#include <iostream>
#include <vector>

class NoBuffDrawable {
public:
    NoBuffDrawable(std::string vertPath, std::string fragPath);
    ~NoBuffDrawable();
    virtual void draw(const Camera &camera) const = 0;

protected:
    GLuint program = 0;
};

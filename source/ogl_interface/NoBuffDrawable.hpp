#ifndef NOBUFFDRAWABLE_HPP
#define NOBUFFDRAWABLE_HPP

#include "Camera.hpp"

#include <vector>
#include <iostream>
#include <fstream>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <epoxy/gl.h>
#include <epoxy/glx.h>

class NoBuffDrawable {
public:
    NoBuffDrawable(std::string vertPath, std::string fragPath);
    ~NoBuffDrawable();
    virtual void draw(const Camera &camera) const = 0;

protected:
    GLuint program = 0;
};

#endif // NOBUFFDRAWABLE_HPP

#pragma once

#include <glm/glm.hpp>

#include <string>
#include <vector>

class Light {
public:
    glm::vec3 pos;
    glm::vec3 color;
    float intensity;

    Light(std::string spec);
    std::vector<unsigned char> getByteColor() const;
};

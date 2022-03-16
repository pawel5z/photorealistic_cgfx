#ifndef LIGHT_HPP
#define LIGHT_HPP

#include <string>
#include <vector>
#include <glm/glm.hpp>

class Light {
public:
    glm::vec3 pos;
    glm::vec3 color;
    float intensity;

    Light(std::string spec);
};

#endif // LIGHT_HPP

#ifndef LIGHT_HPP
#define LIGHT_HPP

#include <glm/glm.hpp>
#include <string>
#include <vector>

class Light {
public:
    glm::dvec3 pos;
    glm::vec3 color;
    float intensity;

    Light(std::string spec);
    std::vector<unsigned char> getByteColor() const;
};

#endif // LIGHT_HPP

#include <cstdio>
#include <stdexcept>

#include "Light.hpp"

Light::Light(std::string spec) {
    if (sscanf(spec.c_str(), "L %f %f %f %f %f %f %f", &pos.x, &pos.y, &pos.z, &color.r, &color.g,
               &color.b, &intensity) < 7)
        throw std::invalid_argument("Could not parse light specs: " + spec);
    color /= 255.f;
}

std::vector<unsigned char> Light::getByteColor() const {
    std::vector<unsigned char> bytes;
    for (int i = 0; i < color.length(); i++)
        bytes.push_back((unsigned char)(color[i] * 255.f));
    return bytes;
}

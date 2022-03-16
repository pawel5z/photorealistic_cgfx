#ifndef MATERIAL_HPP
#define MATERIAL_HPP

#include <assimp/material.h>
#include <glm/glm.hpp>

class Material {
public:
    std::string name;
    float ns; // specular exponent
    float ni; // index of refraction
    unsigned char illum; // illumination model
    glm::vec3 ka; // ambient color
    glm::vec3 kd; // diffuse color
    glm::vec3 ks; // specular color
    glm::vec3 ke; // emissive color

    Material(float ns, float ni, char illum, glm::vec3 ka, glm::vec3 kd, glm::vec3 ks, glm::vec3 ke);
    Material(aiMaterial *mat);
};

#endif // MATERIAL_HPP

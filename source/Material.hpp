#pragma once

#include <assimp/material.h>
#include <glm/glm.hpp>

class Material {
public:
    std::string name;
    float ns;        // specular exponent
    float ni;        // index of refraction
    glm::vec3 ka;    // ambient color
    glm::vec3 kd;    // diffuse color
    glm::vec3 ks;    // specular color
    glm::vec3 ke;    // emissive color
    float roughness; // inferred from ns

    Material(float ns, float ni, glm::vec3 ka, glm::vec3 kd, glm::vec3 ks, glm::vec3 ke);
    Material(aiMaterial *mat);
};

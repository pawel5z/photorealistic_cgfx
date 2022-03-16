#include <iostream>
#include <stdexcept>

#include "Material.hpp"

Material::Material(float ns, float ni, char illum, glm::vec3 ka, glm::vec3 kd,
                   glm::vec3 ks, glm::vec3 ke) : ns(ns), ni(ni), illum(illum), ka(ka), kd(kd),
                                                 ks(ks), ke(ke) {}

Material::Material(aiMaterial *mat) {
    aiString aiName;
    mat->Get(AI_MATKEY_NAME, name);
    name = aiName.data;
    if (AI_SUCCESS != mat->Get(AI_MATKEY_SHININESS, ns))
        throw std::logic_error("Could not get specular exponent.");
    if (AI_SUCCESS != mat->Get(AI_MATKEY_REFRACTI, ni))
        throw std::logic_error("Could not get index of refraction.");
    if (AI_SUCCESS != mat->Get(AI_MATKEY_SHADING_MODEL, illum))
        std::cerr << "Could not get illumination model, but we can go without it.\n";

    aiColor3D col;
    if (AI_SUCCESS != mat->Get(AI_MATKEY_COLOR_AMBIENT, col))
        throw std::logic_error("Could not get ambient color.");
    ka = {col.r, col.g, col.b};
    if (AI_SUCCESS != mat->Get(AI_MATKEY_COLOR_DIFFUSE, col))
        throw std::logic_error("Could not get diffuse color.");
    kd = {col.r, col.g, col.b};
    if (AI_SUCCESS != mat->Get(AI_MATKEY_COLOR_SPECULAR, col))
        throw std::logic_error("Could not get specular color.");
    ks = {col.r, col.g, col.b};
    if (AI_SUCCESS != mat->Get(AI_MATKEY_COLOR_EMISSIVE, col))
        throw std::logic_error("Could not get emissive color.");
    ke = {col.r, col.g, col.b};
}

#include "BRDFs.hpp"

#include <glm/ext.hpp>

#include <algorithm>

glm::vec3 cookTorrance(const glm::vec3 &incoming, const glm::vec3 &outgoing, const glm::vec3 &n,
                       const Material &mat) {
    glm::vec3 half = glm::normalize(incoming + outgoing);
    float cosThetaI = glm::dot(incoming, n), thetaH = glm::acos(glm::dot(half, n)),
          cosThetaO = glm::dot(outgoing, n), cosBeta = glm::dot(half, outgoing);
    return mat.kd * glm::one_over_pi<float>() +
           mat.ks * fresnel(cosBeta, mat.ni) * beckmannDist(thetaH, mat.roughness) *
               geometryMaskingAndShadowing(thetaH, cosThetaI, cosThetaO, cosBeta) /
               (glm::pi<float>() * cosThetaI * cosThetaO);
}

float beckmannDist(float thetaH, float roughness) {
    return glm::exp(-glm::pow(glm::tan(thetaH) / roughness, 2.f)) /
           (roughness * roughness * glm::pow(glm::cos(thetaH), 4.f));
}

float geometryMaskingAndShadowing(float thetaH, float cosThetaI, float cosThetaO, float cosBeta) {
    float twoCosThetaHOverCosBeta = 2.f * glm::cos(thetaH) / cosBeta;
    return std::min(
        {1.f, twoCosThetaHOverCosBeta * cosThetaI, twoCosThetaHOverCosBeta * cosThetaO});
}

float fresnel(float cosBeta, float refrIdx) {
    float f0 = glm::pow((1.f - refrIdx) / (1.f + refrIdx), 2.f);
    return f0 + (1 - f0) * glm::pow(1 - cosBeta, 5.f);
}

glm::vec3 phongModified(const glm::vec3 &incoming, const glm::vec3 &outgoing, const glm::vec3 &n,
                        const Material &mat) {
    return mat.kd / glm::pi<float>() +
           mat.ks * (mat.ns + 2.f) *
               glm::pow(glm::clamp(glm::dot(outgoing, glm::reflect(-incoming, n)), 0.f, 1.f),
                        mat.ns) *
               glm::one_over_two_pi<float>();
}

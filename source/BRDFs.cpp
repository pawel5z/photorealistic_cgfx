#include <algorithm>
#include <glm/ext.hpp>

#include "BRDFs.hpp"

glm::vec3 cookTorrance(const glm::vec3 &incoming, const glm::vec3 &outgoing, const Material &mat) {
    glm::vec3 half = glm::normalize(incoming + outgoing);
    float thetaI = glm::acos(incoming.y), thetaH = glm::acos(half.y),
          thetaO = glm::acos(outgoing.y), beta = glm::acos(glm::dot(half, outgoing));
    return mat.kd * glm::one_over_pi<float>() +
           mat.ks * fresnel(beta, mat.ni) * beckmannDist(thetaH, .5f) *
               geometryMaskingAndShadowing(thetaH, thetaI, thetaO, beta) /
               (glm::pi<float>() * glm::cos(thetaI) * glm::cos(thetaO));
}

float beckmannDist(float thetaH, float roughness) {
    float sqRoughness = roughness * roughness;
    return glm::exp(-glm::pow(glm::tan(thetaH), 2.f) / sqRoughness) /
           (sqRoughness * glm::pow(glm::cos(thetaH), 4.f));
}

float geometryMaskingAndShadowing(float thetaH, float thetaI, float thetaO, float beta) {
    float twoCosThetaHOverCosBeta = 2.f * glm::cos(thetaH) / glm::cos(beta);
    return std::min({1.f, twoCosThetaHOverCosBeta * glm::cos(thetaI),
                     twoCosThetaHOverCosBeta * glm::cos(thetaO)});
}

float fresnel(float beta, float refrIdx) {
    float f0 = glm::pow((1.f - refrIdx) / (1.f + refrIdx), 2.f);
    return f0 + (1 - f0) * glm::pow(1 - glm::cos(beta), 5.f);
}

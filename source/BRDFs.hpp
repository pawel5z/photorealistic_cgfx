#pragma once

#include <glm/glm.hpp>

#include "Material.hpp"

glm::vec3 cookTorrance(const glm::vec3 &incoming, const glm::vec3 &outgoing, const glm::vec3 &n,
                       const Material &mat);
float beckmannDist(float thetaH, float roughness);
float geometryMaskingAndShadowing(float thetaH, float thetaI, float thetaO, float beta);
float fresnel(float beta, float refrIdx);
glm::vec3 phongModified(const glm::vec3 &incoming, const glm::vec3 &outgoing, const glm::vec3 &n,
                        const Material &mat);

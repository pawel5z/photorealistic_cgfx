#pragma once

#include "Material.hpp"

#include <assimp/mesh.h>
#include <glm/glm.hpp>

#include <vector>

struct Vertex {
    glm::vec3 pos;
    glm::vec3 norm;
};

struct Triangle {
    unsigned int indices[3];

    glm::vec3 getCenter(const std::vector<Vertex> &vertices) const;
    float area(const std::vector<Vertex> &vertices) const;
};

class Mesh {
public:
    const unsigned int firstTriangleIdx;
    const unsigned int trianglesCnt;
    const unsigned int matIdx;

    Mesh(const unsigned int firstTriangleIdx, const unsigned int trianglesCnt,
         const unsigned int matIdx);
};

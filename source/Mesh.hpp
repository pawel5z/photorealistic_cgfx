#ifndef MESH_HPP
#define MESH_HPP

#include <assimp/mesh.h>
#include <glm/glm.hpp>
#include <vector>

#include "Material.hpp"

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

#endif // MESH_HPP

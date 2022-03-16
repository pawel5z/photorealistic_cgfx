#ifndef MESH_HPP
#define MESH_HPP

#include <vector>
#include <glm/glm.hpp>
#include <assimp/mesh.h>

#include "Material.hpp"

struct Vertex {
    glm::vec3 pos;
    glm::vec3 norm;
};

struct Triangle {
    unsigned int indices[3];
};

class Mesh {
public:
    std::vector<Vertex> vertices;
    std::vector<Triangle> triangles;
    const Material *mat;

    Mesh(aiMesh *mesh, const std::vector<Material> &mats);
};

#endif // MESH_HPP

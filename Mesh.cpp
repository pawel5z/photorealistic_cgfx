#include "Mesh.hpp"

glm::vec3 Triangle::getCenter(const std::vector<Vertex> &vertices) const {
    return (vertices.at(indices[0]).pos + vertices.at(indices[1]).pos +
            vertices.at(indices[2]).pos) /
           3.f;
}

Mesh::Mesh(const unsigned int firstTriangleIdx, const unsigned int trianglesCnt,
           const unsigned int matIdx)
    : firstTriangleIdx(firstTriangleIdx), trianglesCnt(trianglesCnt), matIdx(matIdx) {}

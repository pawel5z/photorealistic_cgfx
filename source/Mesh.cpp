#include "Mesh.hpp"

glm::vec3 Triangle::getCenter(const std::vector<Vertex> &vertices) const {
    return (vertices.at(indices[0]).pos + vertices.at(indices[1]).pos +
            vertices.at(indices[2]).pos) /
           3.f;
}

float Triangle::area(const std::vector<Vertex> &vertices) const {
    glm::vec3 a = vertices.at(indices[0]).pos, b = vertices.at(indices[1]).pos,
              c = vertices.at(indices[2]).pos;
    return glm::length(glm::cross(b - a, c - a)) * .5f;
}

Mesh::Mesh(const unsigned int firstTriangleIdx, const unsigned int trianglesCnt,
           const unsigned int matIdx)
    : firstTriangleIdx(firstTriangleIdx), trianglesCnt(trianglesCnt), matIdx(matIdx) {}

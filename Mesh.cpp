#include "Mesh.hpp"

Mesh::Mesh(const unsigned int firstTriangleIdx, const unsigned int trianglesCnt,
           const unsigned int matIdx)
    : firstTriangleIdx(firstTriangleIdx), trianglesCnt(trianglesCnt), matIdx(matIdx) {}

#include "KDTree.hpp"

#include <cmath>
#include <stdexcept>

void KDTreeNode::initLeaf() {}

void KDTreeNode::initInterior(unsigned int axis, float split, unsigned int aboveChild) {
    if (axis > 2)
        throw std::invalid_argument("Incorrect split axis argument: " + std::to_string(axis));
    flags = axis;
    this->split = split;
    this->aboveChild |= aboveChild << 2;
}

float KDTreeNode::getSplitPos() const { return split; }

unsigned int KDTreeNode::getTrianglesCnt() const { return trianglesCnt >> 2; }

unsigned int KDTreeNode::getSplitAxis() const { return flags & 0b11u; }

bool KDTreeNode::isLeaf() const { return flags & 0b11u == 0b11u; }

unsigned int KDTreeNode::getAboveChild() const { return aboveChild >> 2; }

KDTree::KDTree(const std::vector<Triangle> &triangles, const std::vector<Vertex> &vertices,
               unsigned int maxLeafCapacity)
    : maxLeafCapacity(maxLeafCapacity),
      maxDepth(std::round(8 + 1.3f * std::log2(triangles.size()))) {

    std::vector<unsigned int> triangleIndices(triangles.size());
    buildTree(triangles, vertices, triangleIndices, maxDepth);
}

void KDTree::buildTree(const std::vector<Triangle> &triangles, const std::vector<Vertex> &vertices,
                       const std::vector<unsigned int> &trianglesIndices, unsigned int depth) {
    // TODO implement
}

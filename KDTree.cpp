#include "KDTree.hpp"

#include <cmath>
#include <random>
#include <stdexcept>

////////////////////////////////////////////////////////////////////////////////
// KDTreeNode
////////////////////////////////////////////////////////////////////////////////

void KDTreeNode::initLeaf(const std::vector<unsigned int> &trianglesIndices,
                          std::vector<unsigned int> &leavesElementsIndices) {
    flags = 0b11u;
    trianglesCnt |= trianglesIndices.size() << 2;
    switch (trianglesIndices.size()) {
    case 0:
        oneTriangle = 0;
        break;
    case 1:
        oneTriangle = trianglesIndices[0];
        break;
    default:
        triangleIndicesOffset = leavesElementsIndices.size();
        leavesElementsIndices.insert(leavesElementsIndices.end(), trianglesIndices.begin(),
                                     trianglesIndices.end());
        break;
    }
}

void KDTreeNode::initInterior(unsigned int axis, float split) {
    if (axis > 2)
        throw std::invalid_argument("Incorrect split axis argument: " + std::to_string(axis));
    flags = axis;
    this->split = split;
}

float KDTreeNode::getSplitPos() const { return split; }

unsigned int KDTreeNode::getTrianglesCnt() const { return trianglesCnt >> 2; }

unsigned int KDTreeNode::getSplitAxis() const { return flags & 0b11u; }

bool KDTreeNode::isLeaf() const { return flags & 0b11u == 0b11u; }

unsigned int KDTreeNode::getAboveChild() const { return aboveChild >> 2; }

void KDTreeNode::setAboveChild(unsigned int idx) { aboveChild = idx << 2 | flags & 0b11u; }

////////////////////////////////////////////////////////////////////////////////
// KDTree
////////////////////////////////////////////////////////////////////////////////

KDTree::KDTree(const std::vector<Triangle> &triangles, const std::vector<Vertex> &vertices,
               unsigned int maxLeafCapacity)
    : maxLeafCapacity(maxLeafCapacity),
      maxDepth(std::round(8 + 1.3f * std::log2(triangles.size()))) {

    std::vector<unsigned int> trianglesIndices(triangles.size());
    std::iota(trianglesIndices.begin(), trianglesIndices.end(), 0);
    buildTree(triangles, vertices, trianglesIndices, maxDepth, ~0u, false, 0);
}

void KDTree::buildTree(const std::vector<Triangle> &triangles, const std::vector<Vertex> &vertices,
                       const std::vector<unsigned int> &trianglesIndices, unsigned int depth,
                       unsigned int parentNodeIdx, bool aboveSplit, unsigned int axis) {
    KDTreeNode node;

    if (trianglesIndices.size() <= maxLeafCapacity || depth == 0) {
        // create leaf node
        node.initLeaf(trianglesIndices, leavesElementsIndices);
        nodes.push_back(node);
        if (parentNodeIdx != ~0u && aboveSplit)
            nodes[parentNodeIdx].setAboveChild(nodes.size() - 1);
        return;
    }

    // create interior node
    // TODO implement better method for selecting split plane
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<unsigned int> indDistrib(0, trianglesIndices.size() - 1);
    std::uniform_int_distribution<unsigned int> vertDistrib(0, 2);
    float split =
        vertices[triangles[trianglesIndices[indDistrib(gen)]].indices[vertDistrib(gen)]].pos[axis];

    std::vector<unsigned int> trianglesIndicesBelow, trianglesIndicesAbove;
    for (auto i : trianglesIndices) {
        Triangle t = triangles[i];
        bool pushedBelow = false, pushedAbove = false;
        for (int j = 0; j < 3; j++) {
            const Vertex &v = vertices[t.indices[j]];
            if (v.pos[axis] <= split) {
                if (!pushedBelow) {
                    trianglesIndicesBelow.push_back(i);
                    pushedBelow = true;
                }
            } else {
                if (!pushedAbove) {
                    trianglesIndicesAbove.push_back(i);
                    pushedAbove = true;
                }
            }
        }
    }

    node.initInterior(axis, split);
    nodes.push_back(node);
    unsigned int nodeIdx = nodes.size() - 1;
    buildTree(triangles, vertices, trianglesIndicesBelow, depth - 1, nodeIdx, false,
              (axis + 1) % 3);
    buildTree(triangles, vertices, trianglesIndicesAbove, depth - 1, nodeIdx, true, (axis + 1) % 3);
}

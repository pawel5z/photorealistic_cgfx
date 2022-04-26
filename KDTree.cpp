#include "KDTree.hpp"

#include <cmath>
#include <glm/gtx/intersect.hpp>
#include <glm/gtx/norm.hpp>
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
        oneTriangle = trianglesIndices.at(0);
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

bool KDTree::findNearestIntersection(Ray r, const std::vector<Triangle> &triangles,
                                     const std::vector<Vertex> &vertices, float &t, glm::vec3 &n,
                                     unsigned int &trianIdx) const {
    return findNearestIntersection(r, triangles, vertices, 0, t, n, trianIdx);
}

bool KDTree::isObstructed(Ray r, const Light &l, const std::vector<Triangle> &triangles,
                          const std::vector<Vertex> &vertices) const {
    return isObstructed(r, l, triangles, vertices, 0);
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
            nodes.at(parentNodeIdx).setAboveChild(nodes.size() - 1);
        return;
    }

    // create interior node
    // TODO implement better method for selecting split plane
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<unsigned int> indDistrib(0, trianglesIndices.size() - 1);
    std::uniform_int_distribution<unsigned int> vertDistrib(0, 2);
    float split =
        vertices.at(triangles.at(trianglesIndices.at(indDistrib(gen))).indices[vertDistrib(gen)])
            .pos[axis];

    std::vector<unsigned int> trianglesIndicesBelow, trianglesIndicesAbove;
    for (auto i : trianglesIndices) {
        Triangle t = triangles.at(i);
        bool pushedBelow = false, pushedAbove = false;
        for (int j = 0; j < 3; j++) {
            const Vertex &v = vertices.at(t.indices[j]);
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

bool KDTree::findNearestIntersection(Ray r, const std::vector<Triangle> &triangles,
                                     const std::vector<Vertex> &vertices, unsigned int nodeIdx,
                                     float &t, glm::vec3 &n, unsigned int &trianIdx) const {
    if (r.tMax < r.tMin)
        return false;

    const KDTreeNode &node = nodes.at(nodeIdx);

    if (node.isLeaf()) {
        float tNearest = std::numeric_limits<float>::max();
        glm::vec2 baryPos;
        for (int i = 0; i < node.getTrianglesCnt(); i++) {
            const Triangle &tri =
                triangles.at(leavesElementsIndices.at(node.triangleIndicesOffset + i));
            Vertex a = vertices.at(tri.indices[0]);
            Vertex b = vertices.at(tri.indices[1]);
            Vertex c = vertices.at(tri.indices[2]);
            if (!glm::intersectRayTriangle(r.o, r.d, a.pos, b.pos, c.pos, baryPos, t))
                continue;
            if (r.tMin + .001f < t && t < r.tMax && t < tNearest) {
                tNearest = t;
                n = a.norm + baryPos.x * (b.norm - a.norm) + baryPos.y * (c.norm - a.norm);
                trianIdx = leavesElementsIndices.at(node.triangleIndicesOffset + i);
            }
        }
        if (tNearest == std::numeric_limits<float>::max())
            return false;
        t = tNearest;
        n = glm::normalize(n);
        return true;
    }

    // interior node
    unsigned int splitAxis = node.getSplitAxis(), firstChildIdx, secondChildIdx;
    bool below = r.o[splitAxis] < node.getSplitPos() ||
                 r.o[splitAxis] == node.getSplitPos() && r.d[splitAxis] <= 0;
    if (below) {
        firstChildIdx = nodeIdx + 1;
        secondChildIdx = node.getAboveChild();
    } else {
        firstChildIdx = node.getAboveChild();
        secondChildIdx = nodeIdx + 1;
    }
    float tPlane = (node.getSplitPos() - r.o[splitAxis]) / r.d[splitAxis];
    if (tPlane > r.tMax || tPlane <= 0)
        return findNearestIntersection(r, triangles, vertices, firstChildIdx, t, n, trianIdx);
    else if (tPlane < r.tMin)
        return findNearestIntersection(r, triangles, vertices, secondChildIdx, t, n, trianIdx);
    else {
        if (findNearestIntersection(Ray(r.o, r.d, r.tMin, tPlane), triangles, vertices,
                                    firstChildIdx, t, n, trianIdx))
            return true;
        else
            return findNearestIntersection(Ray(r.o, r.d, tPlane, r.tMax), triangles, vertices,
                                           secondChildIdx, t, n, trianIdx);
    }
}

bool KDTree::isObstructed(Ray r, const Light &l, const std::vector<Triangle> &triangles,
                          const std::vector<Vertex> &vertices, unsigned int nodeIdx) const {
    if (r.tMax < r.tMin)
        return false;

    const KDTreeNode &node = nodes.at(nodeIdx);

    if (node.isLeaf()) {
        float t;
        glm::vec2 baryPos;
        for (int i = 0; i < node.getTrianglesCnt(); i++) {
            const Triangle &tri =
                triangles.at(leavesElementsIndices.at(node.triangleIndicesOffset + i));
            Vertex a = vertices.at(tri.indices[0]);
            Vertex b = vertices.at(tri.indices[1]);
            Vertex c = vertices.at(tri.indices[2]);
            if (!glm::intersectRayTriangle(r.o, r.d, a.pos, b.pos, c.pos, baryPos, t))
                continue;
            if (r.tMin + .001f < t &&
                glm::distance2(r.o, r.o + r.d * t) < glm::distance2(r.o, l.pos)) {
                return true;
            }
        }
    }

    // interior node
    unsigned int splitAxis = node.getSplitAxis(), firstChildIdx, secondChildIdx;
    bool below = r.o[splitAxis] < node.getSplitPos() ||
                 r.o[splitAxis] == node.getSplitPos() && r.d[splitAxis] <= 0;
    if (below) {
        firstChildIdx = nodeIdx + 1;
        secondChildIdx = node.getAboveChild();
    } else {
        firstChildIdx = node.getAboveChild();
        secondChildIdx = nodeIdx + 1;
    }
    float tPlane = (node.getSplitPos() - r.o[splitAxis]) / r.d[splitAxis];
    if (tPlane > r.tMax || tPlane <= 0)
        return isObstructed(r, l, triangles, vertices, firstChildIdx);
    else if (tPlane < r.tMin)
        return isObstructed(r, l, triangles, vertices, secondChildIdx);
    else {
        if (isObstructed(Ray(r.o, r.d, r.tMin, tPlane), l, triangles, vertices, firstChildIdx))
            return true;
        else
            return isObstructed(Ray(r.o, r.d, tPlane, r.tMax), l, triangles, vertices,
                                secondChildIdx);
    }
}

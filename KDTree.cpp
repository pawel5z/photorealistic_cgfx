#include "KDTree.hpp"

#include <cmath>
#include <glm/gtx/intersect.hpp>
#include <glm/gtx/norm.hpp>
#include <iostream>
#include <random>
#include <stdexcept>

////////////////////////////////////////////////////////////////////////////////
// KDTreeNode
////////////////////////////////////////////////////////////////////////////////

void KDTreeNode::initLeaf(const std::vector<unsigned int> &trianglesIndices,
                          std::vector<unsigned int> &leavesElementsIndices) {
    flags = 0b11u;
    trianglesCnt |= trianglesIndices.size() << 2;
    triangleIndicesOffset = leavesElementsIndices.size();
    leavesElementsIndices.insert(leavesElementsIndices.end(), trianglesIndices.begin(),
                                 trianglesIndices.end());
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

bool KDTreeNode::isLeaf() const { return (flags & 0b11u) == 0b11u; }

unsigned int KDTreeNode::getAboveChild() const { return aboveChild >> 2; }

void KDTreeNode::setAboveChild(unsigned int idx) { aboveChild = idx << 2 | flags & 0b11u; }

////////////////////////////////////////////////////////////////////////////////
// KDTree
////////////////////////////////////////////////////////////////////////////////

KDTree::KDTree(const std::vector<Triangle> &triangles, const std::vector<Vertex> &vertices,
               unsigned int maxLeafCapacity)
    : maxLeafCapacity(maxLeafCapacity),
      maxDepth(std::round(8 + 1.3f * std::log2(triangles.size()))),
      spaceBounds(BBox(triangles.at(0), vertices)) {

    std::cerr << "Building acceleration structure...\n";
    std::vector<unsigned int> trianglesIndices(triangles.size());
    std::iota(trianglesIndices.begin(), trianglesIndices.end(), 0);

    std::vector<BBox> trianglesBounds(triangles.size());
    trianglesBounds.at(0) = spaceBounds;
    for (unsigned int i = 1; i < triangles.size(); i++) {
        trianglesBounds.at(i) = BBox(triangles.at(i), vertices);
        spaceBounds += trianglesBounds.at(i);
    }

    buildTree(triangles, vertices, trianglesIndices, maxDepth, ~0u, false, spaceBounds,
              trianglesBounds);
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
                       std::vector<unsigned int> &trianglesIndices, unsigned int depth,
                       unsigned int parentNodeIdx, bool aboveSplit, BBox nodeBounds,
                       const std::vector<BBox> &trianglesBounds) {
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
    unsigned int axis = std::max({0, 1, 2}, [&](unsigned int d1, unsigned int d2) {
        return nodeBounds.dimLength(d1) < nodeBounds.dimLength(d2);
    });
    float split = (nodeBounds.getDimBounds(axis)[0] + nodeBounds.getDimBounds(axis)[1]) / 2.f;

    std::vector<unsigned int> trianglesIndicesBelow, trianglesIndicesAbove;
    for (const unsigned int i : trianglesIndices) {
        if (trianglesBounds[i].axesBounds[axis][0] <= split)
            trianglesIndicesBelow.push_back(i);
        if (trianglesBounds[i].axesBounds[axis][1] >= split)
            trianglesIndicesAbove.push_back(i);
    }

    node.initInterior(axis, split);
    nodes.push_back(node);
    if (parentNodeIdx != ~0u && aboveSplit)
        nodes.at(parentNodeIdx).setAboveChild(nodes.size() - 1);
    unsigned int nodeIdx = nodes.size() - 1;

    BBox belowBounds = nodeBounds, aboveBounds = nodeBounds;
    belowBounds.replaceUpper(axis, split);
    buildTree(triangles, vertices, trianglesIndicesBelow, depth - 1, nodeIdx, false, belowBounds,
              trianglesBounds);
    aboveBounds.replaceLower(axis, split);
    buildTree(triangles, vertices, trianglesIndicesAbove, depth - 1, nodeIdx, true, aboveBounds,
              trianglesBounds);
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
        return false;
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

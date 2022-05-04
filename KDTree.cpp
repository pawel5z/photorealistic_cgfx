/*
 * Implementation based on
 * https://pbr-book.org/3ed-2018/Primitives_and_Intersection_Acceleration/Kd-Tree_Accelerator.
 */

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
    leavesElementsIndicesOffset = leavesElementsIndices.size();
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
               unsigned int maxDepth, unsigned int maxLeafCapacity, float emptyBonus,
               float traversalCost, float isectCost)
    : maxLeafCapacity(maxLeafCapacity), maxDepth(maxDepth),
      spaceBounds(BBox(triangles.at(0), vertices)), emptyBonus(emptyBonus),
      traversalCost(traversalCost), isectCost(isectCost) {

    std::vector<unsigned int> trianglesIndices(triangles.size());
    std::iota(trianglesIndices.begin(), trianglesIndices.end(), 0);

    std::vector<BBox> trianglesBounds(triangles.size());
    trianglesBounds.at(0) = spaceBounds;
    for (unsigned int i = 1; i < triangles.size(); i++) {
        trianglesBounds.at(i) = BBox(triangles.at(i), vertices);
        spaceBounds += trianglesBounds.at(i);
    }

    std::array<std::vector<BoundEdge>, 3> edges({std::vector<BoundEdge>(2 * triangles.size()),
                                                 std::vector<BoundEdge>(2 * triangles.size()),
                                                 std::vector<BoundEdge>(2 * triangles.size())});
    buildTree(vertices, trianglesIndices, maxDepth, ~0u, false, spaceBounds, trianglesBounds, edges,
              0);

    rayRangeBias = .01f;
}

bool KDTree::findNearestIntersection(Ray r, const std::vector<Triangle> &triangles,
                                     const std::vector<Vertex> &vertices, float &t, glm::vec3 &n,
                                     unsigned int &trianIdx) const {
    return findNearestIntersection(r, triangles, vertices, 0, t, n, trianIdx);
}

bool KDTree::isObstructed(Ray r, const Light &l, const float tLight,
                          const std::vector<Triangle> &triangles,
                          const std::vector<Vertex> &vertices) const {
    return isObstructed(r, l, tLight, triangles, vertices, 0);
}

void KDTree::buildTree(const std::vector<Vertex> &vertices,
                       const std::vector<unsigned int> &trianglesIndices, unsigned int depth,
                       unsigned int parentNodeIdx, bool aboveSplit, const BBox &nodeBounds,
                       const std::vector<BBox> &trianglesBounds,
                       std::array<std::vector<BoundEdge>, 3> &edges, unsigned int badRefines) {
    if (trianglesIndices.size() <= maxLeafCapacity || depth == 0) {
        createLeafNode(trianglesIndices, parentNodeIdx, aboveSplit);
        return;
    }

    // Try to find the best split.
    std::array<unsigned int, 3> candidateAxes = {0, 1, 2};
    std::sort(candidateAxes.begin(), candidateAxes.end(),
              [&](const unsigned int &a1, const unsigned int &a2) {
                  return nodeBounds.dimLength(a1) > nodeBounds.dimLength(a2);
              });
    unsigned int bestAxis = -1, bestOffset = -1;
    float bestCost = std::numeric_limits<float>::max(),
          oldCost = isectCost * trianglesIndices.size(), totalSA = nodeBounds.surfaceArea();

    for (unsigned int i = 0; i < 3; i++) {
        unsigned int axis = candidateAxes.at(i);

        // Initialize edges for axis.
        for (unsigned int j = 0; j < trianglesIndices.size(); j++) {
            unsigned int trianIdx = trianglesIndices.at(j);
            edges.at(axis).at(2 * j) =
                BoundEdge(trianglesBounds.at(trianIdx).axesBounds.at(axis)[0], trianIdx, true);
            edges.at(axis).at(2 * j + 1) =
                BoundEdge(trianglesBounds.at(trianIdx).axesBounds.at(axis)[1], trianIdx, false);
        }
        std::sort(edges.at(axis).begin(), edges.at(axis).begin() + trianglesIndices.size() * 2,
                  [](const BoundEdge &e1, const BoundEdge &e2) {
                      if (e1.t == e2.t)
                          return e1.type < e2.type;
                      return e1.t < e2.t;
                  });

        // Find the best split for axis.
        unsigned int nBelow = 0, nAbove = trianglesIndices.size();
        for (int j = 0; j < 2 * trianglesIndices.size(); j++) {
            if (edges.at(axis).at(j).type == EdgeType::End)
                nAbove--;
            float edgeT = edges.at(axis).at(j).t;
            if (edgeT > nodeBounds.axesBounds.at(axis)[0] &&
                edgeT < nodeBounds.axesBounds.at(axis)[1]) {
                // Compute cost for split at j-th edge.
                unsigned int otherAxis0 = (axis + 1) % 3, otherAxis1 = (axis + 2) % 3;
                float belowSA =
                    2 * (nodeBounds.dimLength(otherAxis0) * nodeBounds.dimLength(otherAxis1) +
                         (edgeT - nodeBounds.axesBounds.at(axis)[0]) *
                             (nodeBounds.dimLength(otherAxis0) + nodeBounds.dimLength(otherAxis1)));
                float aboveSA =
                    2 * (nodeBounds.dimLength(otherAxis0) * nodeBounds.dimLength(otherAxis1) +
                         (nodeBounds.axesBounds.at(axis)[1] - edgeT) *
                             (nodeBounds.dimLength(otherAxis0) + nodeBounds.dimLength(otherAxis1)));
                float pBelow = belowSA / totalSA;
                float pAbove = aboveSA / totalSA;
                float eb = (nAbove == 0 || nBelow == 0) ? emptyBonus : 0;
                float cost =
                    traversalCost + isectCost * (1 - eb) * (pBelow * nBelow + pAbove * nAbove);
                if (cost < bestCost) {
                    bestCost = cost;
                    bestAxis = axis;
                    bestOffset = j;
                }
            }
            if (edges.at(axis).at(j).type == EdgeType::Start)
                nBelow++;
        }

        if (bestAxis == -1 && i < 2)
            continue;

        if (bestCost > oldCost)
            badRefines++;

        if ((bestCost > 4 * oldCost && trianglesIndices.size() < 16) || bestAxis == -1 ||
            badRefines == 3) {
            createLeafNode(trianglesIndices, parentNodeIdx, aboveSplit);
            return;
        } else
            break;
    }

    // Classify triangles with respect to split.
    std::vector<unsigned int> trianglesIndicesBelow, trianglesIndicesAbove;
    for (unsigned int i = 0; i < bestOffset; i++)
        if (edges.at(bestAxis).at(i).type == EdgeType::Start)
            trianglesIndicesBelow.push_back(edges.at(bestAxis).at(i).trianIdx);
    for (unsigned int i = bestOffset + 1; i < 2 * trianglesIndices.size(); i++)
        if (edges.at(bestAxis).at(i).type == EdgeType::End)
            trianglesIndicesAbove.push_back(edges.at(bestAxis).at(i).trianIdx);

    KDTreeNode node;
    float split = edges.at(bestAxis).at(bestOffset).t;
    node.initInterior(bestAxis, split);
    nodes.push_back(node);
    if (parentNodeIdx != ~0u && aboveSplit)
        nodes.at(parentNodeIdx).setAboveChild(nodes.size() - 1);
    unsigned int nodeIdx = nodes.size() - 1;

    BBox belowBounds = nodeBounds, aboveBounds = nodeBounds;
    belowBounds.replaceUpper(bestAxis, split);
    buildTree(vertices, trianglesIndicesBelow, depth - 1, nodeIdx, false, belowBounds,
              trianglesBounds, edges, badRefines);
    aboveBounds.replaceLower(bestAxis, split);
    buildTree(vertices, trianglesIndicesAbove, depth - 1, nodeIdx, true, aboveBounds,
              trianglesBounds, edges, badRefines);
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
                triangles.at(leavesElementsIndices.at(node.leavesElementsIndicesOffset + i));
            Vertex a = vertices.at(tri.indices[0]);
            Vertex b = vertices.at(tri.indices[1]);
            Vertex c = vertices.at(tri.indices[2]);
            if (!glm::intersectRayTriangle(r.o, r.d, a.pos, b.pos, c.pos, baryPos, t))
                continue;
            if (r.tMin + rayRangeBias < t && t < r.tMax + rayRangeBias && t < tNearest) {
                tNearest = t;
                n = a.norm + baryPos.x * (b.norm - a.norm) + baryPos.y * (c.norm - a.norm);
                trianIdx = leavesElementsIndices.at(node.leavesElementsIndicesOffset + i);
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

bool KDTree::isObstructed(Ray r, const Light &l, const float tLight,
                          const std::vector<Triangle> &triangles,
                          const std::vector<Vertex> &vertices, unsigned int nodeIdx) const {
    if (r.tMax < r.tMin)
        return false;

    const KDTreeNode &node = nodes.at(nodeIdx);

    if (node.isLeaf()) {
        float t;
        glm::vec2 baryPos;

        for (int i = 0; i < node.getTrianglesCnt(); i++) {
            const Triangle &tri =
                triangles.at(leavesElementsIndices.at(node.leavesElementsIndicesOffset + i));
            Vertex a = vertices.at(tri.indices[0]);
            Vertex b = vertices.at(tri.indices[1]);
            Vertex c = vertices.at(tri.indices[2]);
            if (!glm::intersectRayTriangle(r.o, r.d, a.pos, b.pos, c.pos, baryPos, t))
                continue;
            if (r.tMin + rayRangeBias < t && t < tLight) {
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
        return isObstructed(r, l, tLight, triangles, vertices, firstChildIdx);
    else if (tPlane < r.tMin)
        return isObstructed(r, l, tLight, triangles, vertices, secondChildIdx);
    else {
        if (isObstructed(Ray(r.o, r.d, r.tMin, tPlane), l, tLight, triangles, vertices,
                         firstChildIdx))
            return true;
        else
            return isObstructed(Ray(r.o, r.d, tPlane, r.tMax), l, tLight, triangles, vertices,
                                secondChildIdx);
    }
}

void KDTree::createLeafNode(const std::vector<unsigned int> &trianglesIndices,
                            unsigned int parentNodeIdx, bool aboveSplit) {
    KDTreeNode node;
    node.initLeaf(trianglesIndices, leavesElementsIndices);
    nodes.push_back(node);
    if (parentNodeIdx != ~0u && aboveSplit)
        nodes.at(parentNodeIdx).setAboveChild(nodes.size() - 1);
}

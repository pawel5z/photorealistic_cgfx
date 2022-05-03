/*
 * Implementation based on
 * https://pbr-book.org/3ed-2018/Primitives_and_Intersection_Acceleration/Kd-Tree_Accelerator.
 */

#ifndef KDTREE_HPP
#define KDTREE_HPP

#include <vector>

#include "BBox.hpp"
#include "BoundEdge.hpp"
#include "Light.hpp"
#include "Mesh.hpp"
#include "Ray.hpp"

struct KDTreeNode {
    union {
        float split;                              // interior
        unsigned int leavesElementsIndicesOffset; // leaf
    };
    union {
        /* 0b00 -- x, 0b01 -- y, 0b10 -- z, 0b11 -- leaf node */
        unsigned int flags; // both
        /* encoded on bits 31--2 */
        unsigned int trianglesCnt; // leaf
        /**
         * Encoded on bits 31--2.
         * Stores index of the child node in nodes vector above the splitting plane.
         * The child node below the splitting plane is just after the current one
         * in the nodes array.
         */
        unsigned int aboveChild; // interior
    };

    void initLeaf(const std::vector<unsigned int> &trianglesIndices,
                  std::vector<unsigned int> &leavesElementsIndices);
    void initInterior(unsigned int axis, float split);
    float getSplitPos() const;
    unsigned int getTrianglesCnt() const;
    unsigned int getSplitAxis() const;
    bool isLeaf() const;
    unsigned int getAboveChild() const;
    void setAboveChild(unsigned int idx);
};

class KDTree {
public:
    KDTree(const std::vector<Triangle> &triangles, const std::vector<Vertex> &vertices,
           unsigned int maxDepth, unsigned int maxLeafCapacity, float emptyBonus,
           float traversalCost, float isectCost);
    bool findNearestIntersection(Ray r, const std::vector<Triangle> &triangles,
                                 const std::vector<Vertex> &vertices, float &t, glm::vec3 &n,
                                 unsigned int &trianIdx) const;
    bool isObstructed(Ray r, const Light &l, const std::vector<Triangle> &triangles,
                      const std::vector<Vertex> &vertices) const;

private:
    std::vector<unsigned int> leavesElementsIndices;
    const unsigned int maxLeafCapacity;
    const unsigned int maxDepth;
    std::vector<KDTreeNode> nodes;
    BBox spaceBounds;
    const float emptyBonus;
    const float traversalCost;
    const float isectCost;

    /**
     * @param axis 0, 1 or 2 (x, y or z respectively).
     */
    void buildTree(const std::vector<Vertex> &vertices,
                   const std::vector<unsigned int> &trianglesIndices, unsigned int depth,
                   unsigned int parentNodeIdx, bool aboveSplit, const BBox &nodeBounds,
                   const std::vector<BBox> &trianglesBounds,
                   std::array<std::vector<BoundEdge>, 3> &edges, unsigned int badRefines);
    bool findNearestIntersection(Ray r, const std::vector<Triangle> &triangles,
                                 const std::vector<Vertex> &vertices, unsigned int nodeIdx,
                                 float &t, glm::vec3 &n, unsigned int &trianIdx) const;
    bool isObstructed(Ray r, const Light &l, const std::vector<Triangle> &triangles,
                      const std::vector<Vertex> &vertices, unsigned int nodeIdx) const;
    void createLeafNode(const std::vector<unsigned int> &trianglesIndices,
                        unsigned int parentNodeIdx, bool aboveSplit);
};

#endif // !KDTREE_HPP

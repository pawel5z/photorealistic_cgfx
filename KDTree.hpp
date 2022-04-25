#ifndef KDTREE_HPP
#define KDTREE_HPP

#include <vector>

#include "Mesh.hpp"

struct KDTreeNode {
    union {
        float split;                        // interior
        unsigned int oneTriangle;           // leaf
        unsigned int triangleIndicesOffset; // leaf
    };
    union {
        /* 0b00 -- x, 0b01 -- y, 0b10 -- z, 0b11 -- leaf node */
        unsigned int flags; // both
        /* encoded on bits 31--2 */
        unsigned int trianglesCnt; // leaf
        /**
         * Encoded on bits 31--2.
         * Stores position of the child node above splitting plane.
         * The child node under splitting plane is just after the current one
         * in the nodes array.
         */
        unsigned int aboveChild; // interior
    };

    void initLeaf();
    void initInterior(unsigned int axis, float split, unsigned int aboveChild);
    float getSplitPos() const;
    unsigned int getTrianglesCnt() const;
    unsigned int getSplitAxis() const;
    bool isLeaf() const;
    unsigned int getAboveChild() const;
};

class KDTree {
public:
    KDTree(const std::vector<Triangle> &triangles, const std::vector<Vertex> &vertices,
           unsigned int maxLeafCapacity);
    ~KDTree();

private:
    void buildTree(const std::vector<Triangle> &triangles, const std::vector<Vertex> &vertices,
                   const std::vector<unsigned int> &trianglesIndices, unsigned int depth);

    std::vector<unsigned int> leavesElementsIndices;
    const unsigned int maxLeafCapacity;
    const unsigned int maxDepth;
    std::vector<KDTreeNode> nodes;
};

#endif // !KDTREE_HPP

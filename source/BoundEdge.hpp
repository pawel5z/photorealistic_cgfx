/*
 * Implementation based on
 * https://pbr-book.org/3ed-2018/Primitives_and_Intersection_Acceleration/Kd-Tree_Accelerator.
 */

#ifndef BOUND_EDGE_HPP
#define BOUND_EDGE_HPP

enum EdgeType { Start, End };

class BoundEdge {
public:
    float t;
    unsigned int trianIdx;
    EdgeType type;

    BoundEdge();
    BoundEdge(float t, unsigned int trianIdx, bool starting);
};

#endif // !BOUND_EDGE_HPP

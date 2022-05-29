/*
 * Implementation based on
 * https://pbr-book.org/3ed-2018/Primitives_and_Intersection_Acceleration/Kd-Tree_Accelerator.
 */

#include "BoundEdge.hpp"

BoundEdge::BoundEdge() {}

BoundEdge::BoundEdge(float t, unsigned int trianIdx, bool starting) : t(t), trianIdx(trianIdx) {
    type = starting ? EdgeType::Start : EdgeType::End;
}

#ifndef BBOX_HPP
#define BBOX_HPP

#include <glm/glm.hpp>

#include "Mesh.hpp"

/**
 * @brief Bounding box.
 */
class BBox {
public:
    glm::vec2 x, y, z;

    BBox();
    BBox(const glm::vec2 &x, const glm::vec2 &y, const glm::vec2 &z);
    BBox(const Triangle &t, const std::vector<Vertex> &vertices);
    BBox &operator+=(const BBox &rhs);
    friend BBox operator+(BBox lhs, const BBox &rhs);
    float dimLength(unsigned int dim) const;
    glm::vec2 getDimBounds(unsigned int dim) const;
    void replaceLower(unsigned int dim, float v);
    void replaceUpper(unsigned int dim, float v);
};

#endif // !BBOX_HPP

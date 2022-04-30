#include "BBox.hpp"

BBox::BBox() : axesBounds({glm::vec2(0), glm::vec2(0), glm::vec2(0)}) {}

BBox::BBox(const std::array<glm::vec2, 3> &axesBounds) : axesBounds(axesBounds) {}

BBox::BBox(const glm::vec2 &x, const glm::vec2 &y, const glm::vec2 &z) : axesBounds({x, y, z}) {}

BBox::BBox(const Triangle &t, const std::vector<Vertex> &vertices) {
    axesBounds = {
        glm::vec2(std::min({vertices.at(t.indices[0]).pos.x, vertices.at(t.indices[1]).pos.x,
                            vertices.at(t.indices[2]).pos.x}),
                  std::max({vertices.at(t.indices[0]).pos.x, vertices.at(t.indices[1]).pos.x,
                            vertices.at(t.indices[2]).pos.x})),
        glm::vec2(std::min({vertices.at(t.indices[0]).pos.y, vertices.at(t.indices[1]).pos.y,
                            vertices.at(t.indices[2]).pos.y}),
                  std::max({vertices.at(t.indices[0]).pos.y, vertices.at(t.indices[1]).pos.y,
                            vertices.at(t.indices[2]).pos.y})),
        glm::vec2(std::min({vertices.at(t.indices[0]).pos.z, vertices.at(t.indices[1]).pos.z,
                            vertices.at(t.indices[2]).pos.z}),
                  std::max({vertices.at(t.indices[0]).pos.z, vertices.at(t.indices[1]).pos.z,
                            vertices.at(t.indices[2]).pos.z}))};
}

BBox &BBox::operator+=(const BBox &rhs) {
    axesBounds = {glm::vec2(std::min(axesBounds.at(0)[0], rhs.axesBounds.at(0)[0]),
                            std::max(axesBounds.at(0)[1], rhs.axesBounds.at(0)[1])),
                  glm::vec2(std::min(axesBounds.at(1)[0], rhs.axesBounds.at(1)[0]),
                            std::max(axesBounds.at(1)[1], rhs.axesBounds.at(1)[1])),
                  glm::vec2(std::min(axesBounds.at(2)[0], rhs.axesBounds.at(2)[0]),
                            std::max(axesBounds.at(2)[1], rhs.axesBounds.at(2)[1]))};
    return *this;
}

BBox operator+(BBox lhs, const BBox &rhs) {
    lhs += rhs;
    return lhs;
}

float BBox::dimLength(unsigned int dim) const {
    return axesBounds.at(dim)[1] - axesBounds.at(dim)[0];
}

glm::vec2 BBox::getDimBounds(unsigned int dim) const { return axesBounds.at(dim); }

void BBox::replaceLower(unsigned int dim, float v) { axesBounds.at(dim)[0] = v; }

void BBox::replaceUpper(unsigned int dim, float v) { axesBounds.at(dim)[1] = v; }

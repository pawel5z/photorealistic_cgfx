#include "BBox.hpp"

BBox::BBox() : x(0), y(0), z(0) {}

BBox::BBox(const glm::vec2 &x, const glm::vec2 &y, const glm::vec2 &z) : x(x), y(y), z(z) {}

BBox::BBox(const Triangle &t, const std::vector<Vertex> &vertices) {
    x = glm::vec2(std::min({vertices.at(t.indices[0]).pos.x, vertices.at(t.indices[1]).pos.x,
                            vertices.at(t.indices[2]).pos.x}),
                  std::max({vertices.at(t.indices[0]).pos.x, vertices.at(t.indices[1]).pos.x,
                            vertices.at(t.indices[2]).pos.x}));
    y = glm::vec2(std::min({vertices.at(t.indices[0]).pos.y, vertices.at(t.indices[1]).pos.y,
                            vertices.at(t.indices[2]).pos.y}),
                  std::max({vertices.at(t.indices[0]).pos.y, vertices.at(t.indices[1]).pos.y,
                            vertices.at(t.indices[2]).pos.y}));
    z = glm::vec2(std::min({vertices.at(t.indices[0]).pos.z, vertices.at(t.indices[1]).pos.z,
                            vertices.at(t.indices[2]).pos.z}),
                  std::max({vertices.at(t.indices[0]).pos.z, vertices.at(t.indices[1]).pos.z,
                            vertices.at(t.indices[2]).pos.z}));
}

BBox &BBox::operator+=(const BBox &rhs) {
    x = glm::vec2(std::min(x[0], rhs.x[0]), std::max(x[1], rhs.x[1]));
    y = glm::vec2(std::min(y[0], rhs.y[0]), std::max(y[1], rhs.y[1]));
    z = glm::vec2(std::min(z[0], rhs.z[0]), std::max(z[1], rhs.z[1]));
    return *this;
}

BBox operator+(BBox lhs, const BBox &rhs) {
    lhs += rhs;
    return lhs;
}

float BBox::dimLength(unsigned int dim) const {
    switch (dim) {
    case 0:
        return x[1] - x[0];
        break;

    case 1:
        return y[1] - y[0];
        break;

    case 2:
        return z[1] - z[0];
        break;

    default:
        throw std::invalid_argument("Incorrect dimension: " + std::to_string(dim));
    }
}

glm::vec2 BBox::getDimBounds(unsigned int dim) const {
    switch (dim) {
    case 0:
        return x;
        break;

    case 1:
        return y;
        break;

    case 2:
        return z;
        break;

    default:
        throw std::invalid_argument("Incorrect dimension: " + std::to_string(dim));
    }
}

void BBox::replaceLower(unsigned int dim, float v) {
    switch (dim) {
    case 0:
        x[0] = v;
        break;

    case 1:
        y[0] = v;
        break;

    case 2:
        z[0] = v;
        break;

    default:
        throw std::invalid_argument("Incorrect dimension: " + std::to_string(dim));
    }
}

void BBox::replaceUpper(unsigned int dim, float v) {
    switch (dim) {
    case 0:
        x[1] = v;
        break;

    case 1:
        y[1] = v;
        break;

    case 2:
        z[1] = v;
        break;

    default:
        throw std::invalid_argument("Incorrect dimension: " + std::to_string(dim));
    }
}

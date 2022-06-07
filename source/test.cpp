#include <algorithm>
#include <cassert>
#include <iostream>

#include "HemisphereSampler.hpp"
#include "Mesh.hpp"
#include "utils.hpp"

static void printSamplerResults(HemisphereSampler &&sampler, const Material *mat = nullptr) {
    for (int i = 0; i < 5; i++) {
        auto [v, p] = sampler(mat);
        std::cout << v << ' ' << p << '\n';
    }
}

int main() {
    std::cout << "CosineSampler" << '\n';
    printSamplerResults(CosineSampler());
    std::cout << '\n';

    std::cout << "UniformSampler" << '\n';
    printSamplerResults(UniformSampler());
    std::cout << '\n';

    Material mockMat = Material(25.f, 0.f, glm::vec3(0), glm::vec3(0), glm::vec3(0), glm::vec3(0));
    std::cout << "CosineLobeSampler" << '\n';
    printSamplerResults(CosineLobeSampler(), &mockMat);
    std::cout << '\n';

    std::cout << "Sample rotations" << '\n';
    std::cout << HemisphereSampler::makeSampleRelativeToNormal(glm::vec3(0, 1, 0),
                                                               glm::vec3(1, 0, 0))
              << '\n';
    std::cout << HemisphereSampler::makeSampleRelativeToNormal(
                     glm::vec3(0, glm::sqrt(2.f) / 2.f, glm::sqrt(2.f) / 2.f), glm::vec3(1, 0, 0))
              << '\n';
    std::cout << '\n';

    std::cout << "Area of triangle" << '\n';
    std::vector<Vertex> vertices{{{0, 0, 0}}, {{1, 0, 0}}, {{0, 1, 0}}};
    Triangle trian = {{0, 1, 2}};
    std::cout << trian.area(vertices) << '\n';
}

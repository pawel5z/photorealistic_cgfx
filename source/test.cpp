#include <algorithm>
#include <cassert>
#include <iostream>

#include "HemisphereSampler.hpp"

#include "utils.hpp"

static void printSamplerResults(HemisphereSampler &&sampler) {
    std::cout << "CosineSampler" << '\n';
    for (int i = 0; i < 5; i++) {
        auto [v, p] = sampler();
        std::cout << v << '\n';
    }
}

int main() {
    printSamplerResults(CosineSampler());
    std::cout << '\n';

    printSamplerResults(BeckmannSampler());
    std::cout << '\n';

    printSamplerResults(UniformSampler());
    std::cout << '\n';

    std::cout << HemisphereSampler::makeSampleRelativeToNormal(glm::vec3(0, 1, 0),
                                                               glm::vec3(1, 0, 0))
              << '\n';
    std::cout << HemisphereSampler::makeSampleRelativeToNormal(
                     glm::vec3(0, glm::sqrt(2.f) / 2.f, glm::sqrt(2.f) / 2.f), glm::vec3(1, 0, 0))
              << '\n';
}

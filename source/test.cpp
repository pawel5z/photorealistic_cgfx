#include <algorithm>
#include <iostream>

#include "HemisphereSampler.hpp"

int main() {
    CosineSampler s1;
    for (int i = 0; i < 5; i++) {
        auto [v, p] = s1();
        std::cout << v[0] << ' ' << v[1] << ' ' << v[2] << ' ' << glm::length(v) << ' ' << p
                  << '\n';
    }
    std::cout << '\n';
    BeckmannSampler s2;
    for (int i = 0; i < 5; i++) {
        auto [v, p] = s2();
        std::cout << v[0] << ' ' << v[1] << ' ' << v[2] << ' ' << glm::length(v) << ' ' << p
                  << '\n';
    }
}

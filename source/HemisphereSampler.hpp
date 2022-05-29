#pragma once

#include <glm/glm.hpp>
#include <random>

class HemisphereSampler {
public:
    static glm::vec3 makeSampleRelativeToNormal(const glm::vec3 &s, const glm::vec3 &n);

    virtual std::tuple<glm::vec3, float> operator()() = 0;
    std::tuple<glm::vec3, float> operator()(const glm::vec3 &n);
    virtual float pdf(const glm::vec3 &v) const = 0;
};

class CosineSampler : public HemisphereSampler {
public:
    CosineSampler();
    std::tuple<glm::vec3, float> operator()() override;
    float pdf(const glm::vec3 &v) const override;

protected:
    std::mt19937 randEng;
    std::uniform_real_distribution<float> dist;
};

class BeckmannSampler : public CosineSampler {
public:
    std::tuple<glm::vec3, float> operator()() override;
};

#pragma once

#include "Material.hpp"

#include <glm/glm.hpp>

#include <random>

class HemisphereSampler {
public:
    static glm::vec3 makeSampleRelativeToNormal(const glm::vec3 &s, const glm::vec3 &n);

    std::tuple<glm::vec3, float> operator()(const Material *mat = nullptr);
    std::tuple<glm::vec3, float> operator()(const glm::vec3 &n, const Material *mat = nullptr);
    virtual glm::vec3 sample(const Material *mat = nullptr) = 0;
    virtual float pdf(const glm::vec3 &v, const Material *mat = nullptr) const = 0;
};

class RandomHemisphereSampler : public HemisphereSampler {
public:
    RandomHemisphereSampler();

protected:
    std::mt19937 randEng;
};

class CosineSampler : public RandomHemisphereSampler {
public:
    glm::vec3 sample(const Material *mat = nullptr) override;
    float pdf(const glm::vec3 &v, const Material *mat = nullptr) const override;

private:
    std::uniform_real_distribution<float> dist;
};

class BeckmannSampler : public RandomHemisphereSampler {
public:
    glm::vec3 sample(const Material *mat = nullptr) override;
    float pdf(const glm::vec3 &v, const Material *mat = nullptr) const override;

private:
    std::uniform_real_distribution<float> dist;
};

class UniformSampler : public RandomHemisphereSampler {
public:
    glm::vec3 sample(const Material *mat = nullptr) override;
    float pdf(const glm::vec3 &v, const Material *mat = nullptr) const override;

private:
    std::uniform_real_distribution<float> dist;
};

class CosineLobeSampler : public RandomHemisphereSampler {
public:
    glm::vec3 sample(const Material *mat = nullptr) override;
    float pdf(const glm::vec3 &v, const Material *mat = nullptr) const override;

private:
    std::uniform_real_distribution<float> dist;
};

#include <glm/gtx/quaternion.hpp>
#include <stdexcept>
#include <tuple>

#include "HemisphereSampler.hpp"

// abstract hemisphere sampler

glm::vec3 HemisphereSampler::makeSampleRelativeToNormal(const glm::vec3 &s, const glm::vec3 &n) {
    return glm::rotate(glm::rotation(glm::vec3(0, 1, 0), n), s);
}

std::tuple<glm::vec3, float> HemisphereSampler::operator()(const Material *mat) {
    glm::vec3 s = sample(mat);
    return std::make_tuple(s, pdf(s, mat));
}

std::tuple<glm::vec3, float> HemisphereSampler::operator()(const glm::vec3 &n,
                                                           const Material *mat) {
    auto [s, prob] = (*this)(mat);
    return std::make_tuple(makeSampleRelativeToNormal(s, n), prob);
}

// abstract random hemisphere sampler

RandomHemisphereSampler::RandomHemisphereSampler()
    :
#ifdef DEBUG
      randEng(42)
#else
      randEng(std::random_device()())
#endif // DEBUG
{
}

// cosine sampler

glm::vec3 CosineSampler::sample(const Material *mat) {
    float r2 = dist(randEng);
    float twoPiRand = glm::two_pi<float>() * dist(randEng);
    float sqrtOneMinusRand = glm::sqrt(1.f - r2);
    return glm::vec3(glm::cos(twoPiRand) * sqrtOneMinusRand, glm::sqrt(r2),
                     glm::sin(twoPiRand) * sqrtOneMinusRand);
}

float CosineSampler::pdf(const glm::vec3 &v, const Material *mat) const {
    return v.y * glm::one_over_pi<float>();
}

// Beckmann sampler

glm::vec3 BeckmannSampler::sample(const Material *mat) {
    if (mat == nullptr)
        throw std::invalid_argument("mat is nullptr.");
    float theta =
        glm::atan(glm::sqrt(-mat->roughness * mat->roughness * glm::log(1.f - dist(randEng))));
    float phi = glm::two_pi<float>() * dist(randEng);
    return glm::vec3(glm::sin(theta) * glm::cos(phi), glm::cos(theta),
                     glm::sin(theta) * glm::sin(phi));
}

float BeckmannSampler::pdf(const glm::vec3 &v, const Material *mat) const {
    if (mat == nullptr)
        throw std::invalid_argument("mat is nullptr.");
    float theta = glm::acos(v.y);
    return glm::sin(theta) *
           glm::exp(-glm::pow(glm::tan(theta), 2.f) / (mat->roughness * mat->roughness)) /
           (glm::pi<float>() * mat->roughness * mat->roughness * glm::pow(v.y, 3.f));
}

// uniform sampler

glm::vec3 UniformSampler::sample(const Material *mat) {
    float r2 = dist(randEng);
    float twoPiRandom = glm::two_pi<float>() * dist(randEng),
          sqrtOneSubSqRand = glm::sqrt(1.f - r2 * r2);
    return glm::vec3(glm::cos(twoPiRandom) * sqrtOneSubSqRand, r2,
                     glm::sin(twoPiRandom) * sqrtOneSubSqRand);
}

float UniformSampler::pdf(const glm::vec3 &v, const Material *mat) const {
    return glm::one_over_two_pi<float>();
}

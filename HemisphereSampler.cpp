#include <glm/gtx/quaternion.hpp>
#include <tuple>

#include "HemisphereSampler.hpp"

// abstract hemisphere sampler

glm::vec3 HemisphereSampler::makeSampleRelativeToNormal(const glm::vec3 &s, const glm::vec3 &n) {
    return glm::rotate(glm::rotation(glm::vec3(0, 1, 0), n), s);
}

std::tuple<glm::vec3, float> HemisphereSampler::operator()(const glm::vec3 &n) {
    auto [s, prob] = (*this)();
    return std::make_tuple(makeSampleRelativeToNormal(s, n), prob);
}

// cosine sampler

CosineSampler::CosineSampler() : randEng(std::random_device()()) {}

std::tuple<glm::vec3, float> CosineSampler::operator()() {
    float r1 = dist(randEng), r2 = dist(randEng);
    float twoPiRand = glm::two_pi<float>() * r1;
    float sqrtOneMinusRand = glm::sqrt(1.f - r2);
    glm::vec3 v(glm::cos(twoPiRand) * sqrtOneMinusRand, glm::sqrt(r2),
                glm::sin(twoPiRand) * sqrtOneMinusRand);
    return std::make_tuple(v, pdf(v));
}

float CosineSampler::pdf(const glm::vec3 &v) const {
    return glm::cos(glm::asin(v.y)) / glm::pi<float>();
}

// Beckmann sampler

std::tuple<glm::vec3, float> BeckmannSampler::operator()() {
    float latitude =
        glm::acos(glm::inversesqrt(1.f - .25 * glm::log(1.f - dist(randEng)))); // theta
    float longitude = glm::two_pi<float>() * dist(randEng);                     // phi
    glm::vec3 v(glm::sin(latitude) * glm::cos(longitude), glm::cos(latitude),
                glm::sin(latitude) * glm::sin(longitude));
    return std::make_tuple(v, pdf(v));
}

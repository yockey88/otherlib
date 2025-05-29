/**
 * \file math/random.cpp
 **/
#include "math/random.hpp"

#include <random>

#include "math/constants.hpp"

namespace other {

  float rand_float() {
    static std::uniform_real_distribution<float> distribution(0.0, 1.0);
    static std::mt19937 generator;
    return distribution(generator);
  }

  float rand_float(float min, float max) {
    // Returns a random real in [min,max).
    return min + (max - min) * rand_float();
  }

  float linear_to_gamma(float linear_component) {
    if (linear_component > 0) {
      return std::sqrt(linear_component);
    }

    return 0;
  }

  glm::vec3 sample_square() {
    /// Sample a point in the square [-0.5, 0.5] x [-0.5, 0.5]
    return glm::vec3(rand_float() - 0.5f, rand_float() - 0.5f, 0.f);
  }

  glm::vec3 random_vec3() {
    return glm::vec3(rand_float(), rand_float(), rand_float());
  }

  glm::vec3 random_vec3(float min, float max) {
    return glm::vec3(rand_float(min, max), rand_float(min, max), rand_float(min, max));
  }

  glm::vec3 random_unit_vector() {
    do {
      auto p = random_vec3(-1.f, 1.f);
      float lensq = glm::dot(p, p);
      if (epsilon < lensq && lensq <= 1.f) {
        return glm::normalize(p);
      }
    } while (true);
  }

  glm::vec3 random_in_hemisphere(const glm::vec3& normal) {
    glm::vec3 in_unit_sphere = random_unit_vector();
    if (glm::dot(in_unit_sphere, normal) > 0.f) {
      return in_unit_sphere;
    } else {
      return -in_unit_sphere;
    }
  }

}  // namespace other
/**
 * \file math/random.cpp
 **/
#include "math/random.hpp"

#include <random>

#include "math/constants.hpp"

namespace other {

  real_t rand_real_t() {
    static std::uniform_real_distribution<real_t> distribution(0.0, 1.0);
    static std::mt19937 generator;
    return distribution(generator);
  }

  real_t rand_real_t(real_t min, real_t max) {
    // Returns a random real in [min,max).
    return min + (max - min) * rand_real_t();
  }

  glm::vec3 sample_square() {
    /// Sample a point in the square [-0.5, 0.5] x [-0.5, 0.5]
    return glm::vec3(rand_real_t() - 0.5f, rand_real_t() - 0.5f, 0.f);
  }

  glm::vec3 random_vec3() {
    return glm::vec3(rand_real_t(), rand_real_t(), rand_real_t());
  }

  glm::vec3 random_vec3(real_t min, real_t max) {
    return glm::vec3(rand_real_t(min, max), rand_real_t(min, max), rand_real_t(min, max));
  }

  glm::vec3 random_unit_vector() {
    do {
      auto p = random_vec3(-1.f, 1.f);
      real_t lensq = glm::dot(p, p);
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
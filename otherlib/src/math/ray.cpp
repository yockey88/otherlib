/**
 * \file math/ray.cpp
 **/
#include "math/ray.hpp"

namespace other {

  orthonormal_basis::orthonormal_basis(const glm::vec3& n) {
    k = glm::normalize(n);
    i = glm::normalize(glm::cross(k, glm::vec3(0, 1, 0)));
    j = glm::normalize(glm::cross(k, i));
  }

  glm::vec3 orthonormal_basis::to_local(const glm::vec3& v) const {
    return glm::vec3(glm::dot(v, i), glm::dot(v, j), glm::dot(v, k));
  }

  glm::vec3 orthonormal_basis::to_world(const glm::vec3& vec) const {
    return i * vec.x + j * vec.y + k * vec.z;
  }

  glm::mat4 orthonormal_basis::to_matrix() const {
    // clang-format off
    return glm::mat4(i.x, j.x, k.x, 0,
                     i.y, j.y, k.y, 0,
                     i.z, j.z, k.z, 0,
                     0,   0,   0,   1);
    // clang-format on
  }

  interval interval::infinite = { -std::numeric_limits<float>::max(), std::numeric_limits<float>::max() };
  interval interval::empty = { std::numeric_limits<float>::max(), -std::numeric_limits<float>::infinity() };

  glm::vec3 ray::at(float t) const {
    return origin + t * direction;
  }

}  // namespace other
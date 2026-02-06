/**
 * \file math/orthonormal_basis.cpp
 **/
#include "math/orthonormal_basis.hpp"

#include "math/constants.hpp"

namespace other {

  namespace {

    void check_negative_zero(glm::vec3& v) {
      if (v.x == -0.f) {
        v.x = 0.f;
      }
      if (v.y == -0.f) {
        v.y = 0.f;
      }
      if (v.z == -0.f) {
        v.z = 0.f;
      }
    }

  }  // namespace

  orthonormal_basis::orthonormal_basis(const glm::vec3& reference_vector, const glm::vec3& n) {
    j = glm::normalize(n);
    i = glm::normalize(glm::cross(j, reference_vector));
    k = glm::normalize(glm::cross(i, j));
    check_negative_zero();
  }

  orthonormal_basis orthonormal_basis::from_rotation(const glm::quat& rotation) {
    glm::mat3 rot_matrix = glm::toMat3(rotation);
    return from_matrix(rot_matrix);
  }

  orthonormal_basis orthonormal_basis::from_rotation(const glm::vec3& euler_angles) {
    glm::quat rot = glm::quat(euler_angles);
    return from_rotation(rot);
  }

  orthonormal_basis orthonormal_basis::from_matrix(const glm::mat4& matrix) {
    return from_matrix(glm::mat3(matrix));
  }

  orthonormal_basis orthonormal_basis::from_matrix(const glm::mat3& matrix) {
    orthonormal_basis basis;
    basis.i = matrix[0];
    basis.j = matrix[1];
    basis.k = matrix[2];
    basis.check_negative_zero();
    return basis;
  }

  glm::vec3 orthonormal_basis::to_local(const glm::vec3& v) const {
    return glm::vec3(
      satisfy_floating_point_tolerance(glm::dot(v, i)),
      satisfy_floating_point_tolerance(glm::dot(v, j)),
      satisfy_floating_point_tolerance(glm::dot(v, k))
    );
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

  glm::vec3 orthonormal_basis::find_first_non_zero(const glm::vec3& v) const {
    auto vp = glm::vec3(-v.y, v.x, 0.f);
    return (glm::length(vp) < 0.0001f) ? glm::vec3(1.f, 0.f, 0.f) : glm::normalize(vp);
  }

  void orthonormal_basis::check_negative_zero() {
    other::check_negative_zero(i);
    other::check_negative_zero(j);
    other::check_negative_zero(k);
  }

}  // namespace other
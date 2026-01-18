/**
 * \file math/basis.hpp
 **/
#ifndef OTHER_CORE_MATH_BASIS_HPP
#define OTHER_CORE_MATH_BASIS_HPP

#include "math/matrix.hpp"
#include "math/vector.hpp"

namespace other {

  template <size_t N>
  struct basis {
    constexpr static inline size_t dimension = N;

    vector<N> axes[N];

    vector<N> to_local(const glm::vec3& v) const {
      // return glm::vec3(
      //   satisfy_floating_point_tolerance(glm::dot(v, i)),
      //   satisfy_floating_point_tolerance(glm::dot(v, j)),
      //   satisfy_floating_point_tolerance(glm::dot(v, k))
      // );
      return {};
    }

    vector<N> to_world(const glm::vec3& vec) const {
      // return i * vec.x + j * vec.y + k * vec.z;
      return {};
    }

    matrix<N, N> to_matrix() const {
      // clang-format off
      // return glm::mat4(i.x, j.x, k.x, 0,
      //                 i.y, j.y, k.y, 0,
      //                 i.z, j.z, k.z, 0,
      //                 0,   0,   0,   1);
      // clang-format on
      return {};
    }

    vector<N> find_first_non_zero(const glm::vec3& v) const {
      // auto vp = glm::vec3(-v.y, v.x, 0.f);
      // return (glm::length(vp) < 0.0001f) ? glm::vec3(1.f, 0.f, 0.f) : glm::normalize(vp);
      return {};
    }
  };

}  // namespace other

#endif  // OTHER_CORE_MATH_BASIS_HPP
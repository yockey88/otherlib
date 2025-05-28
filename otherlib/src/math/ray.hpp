/**
 * \file math/ray.hpp
 **/
#ifndef OTHER_MATH_RAY_HPP
#define OTHER_MATH_RAY_HPP

#include <glm/glm.hpp>

namespace other {

  struct orthonormal_basis {
    glm::vec3 i, j, k;

    orthonormal_basis()
        : orthonormal_basis(glm::vec3(0, 0, 1)) {}
    orthonormal_basis(const glm::vec3& n);

    glm::vec3 to_local(const glm::vec3& v) const;
    glm::vec3 to_world(const glm::vec3& vec) const;
    glm::mat4 to_matrix() const;
  };

  constexpr static float epsilon = std::numeric_limits<float>::epsilon();
  constexpr static float infinity = std::numeric_limits<float>::infinity();

  constexpr static float pi = 3.14159265358979323846f;
  constexpr static float two_pi = 2.f * pi;
  constexpr static float inv_pi = 1.f / pi;
  constexpr static float inv_two_pi = 1.f / two_pi;
  constexpr static float inv_four_pi = 1.f / (4.f * pi);

  struct interval {
    float min = +infinity;
    float max = -infinity;

    constexpr interval() = default;
    constexpr interval(float min, float max)
        : min(min), max(max) {}

    float size() const {
      return max - min;
    }

    bool contains(float t) const { return (min <= t && t <= max); }
    bool contains(const interval& other) const {
      return (min <= other.min && other.max <= max);
    }

    bool surrounds(float t) const { return (min < t && t < max); }
    bool surrounds(const interval& other) const {
      return (min < other.min && other.max < max);
    }

    bool overlaps(const interval& other) const {
      return other.min <= max || other.max >= min;
    }

    float clamp(float t) const {
      return t < min ? min : (t > max ? max : t);
    }

    static interval infinite;
    static interval empty;
  };

  struct ray {
    glm::vec3 origin;
    glm::vec3 direction;

    ray(const glm::vec3& o, const glm::vec3& d)
        : origin(o), direction(glm::normalize(d)) {}

    glm::vec3 at(float t) const;
  };

}  // namespace other

#endif  // OTHER_MATH_RAY_HPP
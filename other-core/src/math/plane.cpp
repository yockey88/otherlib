/**
 * \file math/plane.cpp
 **/
#include "math/plane.hpp"

#include "math/epsilon_math.hpp"

namespace other {

  float plane::distance_to_point(const glm::vec3& point) const {
    return glm::dot(normal, point) - distance;
  }

  bool plane::is_facing_point(const glm::vec3& point) const {
    return distance_to_point(point) > 0.0f;
  }

  bool plane::contains_point(const glm::vec3& point) const {
    return detail::epsilon_zero(distance_to_point(point));
  }

}  // namespace other
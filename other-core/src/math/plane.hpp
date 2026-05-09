/**
 * \file math/plane.hpp
 **/
#ifndef OTHER_CORE_MATH_PLANE_HPP
#define OTHER_CORE_MATH_PLANE_HPP

#include <glm/glm.hpp>

#include "serialization/reflection.hpp"

namespace other {

  struct plane {
    glm::vec3 normal;
    float distance;

    plane() : normal(0.0f, 1.0f, 0.0f), distance(0.0f) {}
    plane(const glm::vec3& p1, const glm::vec3& normal)
        : normal(glm::normalize(normal)), distance(glm::dot(this->normal, p1)) {}

    float distance_to_point(const glm::vec3& point) const;
    bool is_facing_point(const glm::vec3& point) const;
    bool contains_point(const glm::vec3& point) const;
  };

}  // namespace other

OTHER_REFLECT(
  other::plane,
  field(normal, other::attr::serializable()),
  field(distance, other::attr::serializable())
)

#endif  // OTHER_CORE_MATH_PLANE_HPP
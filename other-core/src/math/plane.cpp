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

  std::array<glm::vec3, 4> plane::get_corner_points() const {
    std::array<glm::vec3, 4> corners;
    glm::vec3 tangent1;
    if (std::abs(normal.x) < std::abs(normal.y) && std::abs(normal.x) < std::abs(normal.z)) {
      tangent1 = glm::vec3(0.0f, -normal.z, normal.y);
    } else if (std::abs(normal.y) < std::abs(normal.z)) {
      tangent1 = glm::vec3(-normal.z, 0.0f, normal.x);
    } else {
      tangent1 = glm::vec3(-normal.y, normal.x, 0.0f);
    }
    tangent1 = glm::normalize(tangent1);
    glm::vec3 tangent2 = glm::normalize(glm::cross(normal, tangent1));

    float plane_size = 1000.0f;  // arbitrary large size for the plane
    corners[0] = normal * distance + tangent1 * plane_size + tangent2 * plane_size;
    corners[1] = normal * distance + tangent1 * plane_size - tangent2 * plane_size;
    corners[2] = normal * distance - tangent1 * plane_size - tangent2 * plane_size;
    corners[3] = normal * distance - tangent1 * plane_size + tangent2 * plane_size;

    return corners;
  }

}  // namespace other
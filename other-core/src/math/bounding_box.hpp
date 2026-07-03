/**
 * \file math/bounding_box.hpp
 **/
#ifndef OTHER_CORE_MATH_BOUNDING_BOX_HPP
#define OTHER_CORE_MATH_BOUNDING_BOX_HPP

#include <glm/glm.hpp>

#include "math/ray.hpp"
#include "serialization/reflection.hpp"

namespace other {

  struct bounding_box {
    glm::vec3 min = glm::vec3(std::numeric_limits<float>::max());
    glm::vec3 max = glm::vec3(-std::numeric_limits<float>::max());

    bounding_box() = default;
    bounding_box(const glm::vec3& min, const glm::vec3& max)
        : min(min), max(max) {}

    static bounding_box empty;
    static bounding_box infinite;

    constexpr auto operator<=>(const bounding_box& other) const {
      return min.x >= other.min.x && min.y >= other.min.y && min.z >= other.min.z &&
        max.x <= other.max.x && max.y <= other.max.y && max.z <= other.max.z;
    }
    inline bool operator==(const bounding_box& other) const = default;

    bool contains(const glm::vec3& point) const;

    bool intersects(const bounding_box& other) const;
    bool intersects(const ray& r) const;

    bounding_box transform(const glm::mat4& t);

    static bounding_box expand_to_include(const bounding_box& box, const bounding_box& other);
  };

}  // namespace other

OTHER_REFLECT(
  other::bounding_box,
  field(min, other::attr::serializable()),
  field(max, other::attr::serializable()))

#endif  // OTHER_CORE_MATH_BOUNDING_BOX_HPP
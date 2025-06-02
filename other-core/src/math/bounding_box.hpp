/**
 * \file math/bounding_box.hpp
 **/
#ifndef OTHER_CORE_MATH_BOUNDING_BOX_HPP
#define OTHER_CORE_MATH_BOUNDING_BOX_HPP

#include <glm/glm.hpp>

namespace other {

  struct bounding_box {
    glm::vec3 min = { 0, 0, 0 };
    glm::vec3 max = { 0, 0, 0 };

    bounding_box() = default;
    bounding_box(const glm::vec3& min, const glm::vec3& max)
        : min(min), max(max) {}

    static bounding_box empty;
    static bounding_box infinite;

    bool contains(const glm::vec3& point) const;
  };

}  // namespace other

#endif  // OTHER_CORE_MATH_BOUNDING_BOX_HPP
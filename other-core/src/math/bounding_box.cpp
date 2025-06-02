/**
 * \file math/bounding_box.cpp
 **/
#include "math/bounding_box.hpp"

namespace other {

  bool bounding_box::contains(const glm::vec3& point) const {
    return (point.x >= min.x && point.x <= max.x) &&
      (point.y >= min.y && point.y <= max.y) &&
      (point.z >= min.z && point.z <= max.z);
  }

}  // namespace other

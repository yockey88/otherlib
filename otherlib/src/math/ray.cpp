/**
 * \file math/ray.cpp
 **/
#include "math/ray.hpp"

namespace other {

  glm::vec3 ray::at(float t) const {
    return origin + t * direction;
  }

}  // namespace other
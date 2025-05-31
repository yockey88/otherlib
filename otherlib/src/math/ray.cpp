/**
 * \file math/ray.cpp
 **/
#include "math/ray.hpp"

namespace other {

  glm::vec3 ray::at(real_t t) const {
    return origin + t * direction;
  }

}  // namespace other
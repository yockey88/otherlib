/**
 * @file math/ray.hpp
 */
#ifndef OTHER_CORE_MATH_RAY_HPP
#define OTHER_CORE_MATH_RAY_HPP

#include <glm/glm.hpp>

#include "core/defines.hpp"

namespace other {

  struct ray {
    glm::vec3 origin;
    glm::vec3 direction;

    ray(const glm::vec3& o, const glm::vec3& d)
        : origin(o), direction(glm::normalize(d)) {}

    glm::vec3 at(real_t t) const;
  };

}  // namespace other

#endif  // OTHER_CORE_MATH_RAY_HPP
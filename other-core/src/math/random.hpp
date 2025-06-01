/**
 * @file math/random.hpp
 */
#ifndef OTHER_CORE_MATH_RANDOM_HPP
#define OTHER_CORE_MATH_RANDOM_HPP

#include <glm/glm.hpp>

#include "core/defines.hpp"

namespace other {

  real_t rand_float();
  real_t rand_float(real_t min, real_t max);

  glm::vec3 sample_square();
  glm::vec3 random_vec3();
  glm::vec3 random_vec3(real_t min, real_t max);
  glm::vec3 random_unit_vector();
  glm::vec3 random_in_hemisphere(const glm::vec3& normal);

}  // namespace other

#endif  // OTHER_CORE_MATH_RANDOM_HPP
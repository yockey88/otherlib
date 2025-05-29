/**
 * \file math/random.hpp
 **/
#ifndef OTHER_MATH_RANDOM_HPP
#define OTHER_MATH_RANDOM_HPP

#include <glm/glm.hpp>

namespace other {

  float rand_float();
  float rand_float(float min, float max);
  float linear_to_gamma(float linear_component);

  glm::vec3 sample_square();
  glm::vec3 random_vec3();
  glm::vec3 random_vec3(float min, float max);
  glm::vec3 random_unit_vector();
  glm::vec3 random_in_hemisphere(const glm::vec3& normal);

}  // namespace other

#endif  // !OTHER_MATH_RANDOM_HPP
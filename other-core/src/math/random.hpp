/**
 * @file math/random.hpp
 */
#ifndef OTHER_CORE_MATH_RANDOM_HPP
#define OTHER_CORE_MATH_RANDOM_HPP

#include <random>
#include <type_traits>

#include <glm/glm.hpp>

#include "core/defines.hpp"


namespace other {

  template <typename T>
    requires std::is_integral_v<T>
  struct random_generator {
    T min, max;

    std::mt19937 engine;
    std::uniform_int_distribution<T> distribution;

    random_generator()
        : min(std::numeric_limits<T>::min()), max(std::numeric_limits<T>::max()),
          engine(std::random_device{}()), distribution(min, max) {}
    random_generator(const T min, const T max)
        : min(min), max(max), engine(std::random_device{}()), distribution(min, max) {}

    T next() { return distribution(engine); }
  };

  real_t rand_float();
  real_t rand_float(real_t min, real_t max);

  glm::vec3 sample_square();
  glm::vec3 random_vec3();
  glm::vec3 random_vec3(real_t min, real_t max);
  glm::vec3 random_unit_vector();
  glm::vec3 random_in_hemisphere(const glm::vec3& normal);

}  // namespace other

#endif  // OTHER_CORE_MATH_RANDOM_HPP
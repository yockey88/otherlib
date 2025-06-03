/**
 * @file math/constants.hpp
 */
#ifndef OTHER_CORE_MATH_CONSTANTS_HPP
#define OTHER_CORE_MATH_CONSTANTS_HPP

#include <cmath>
#include <limits>

#include <glm/glm.hpp>

#include "core/defines.hpp"

namespace other {

  constexpr static real_t epsilon = std::numeric_limits<real_t>::epsilon();
  constexpr static real_t infinity = std::numeric_limits<real_t>::infinity();

  constexpr static real_t pi = 3.14159265358979323846f;
  constexpr static real_t two_pi = 2.f * pi;
  constexpr static real_t inv_pi = 1.f / pi;
  constexpr static real_t inv_two_pi = 1.f / two_pi;
  constexpr static real_t inv_four_pi = 1.f / (4.f * pi);

  constexpr static inline real_t degrees_to_radians(real_t degrees) {
    return degrees * (pi / 180.f);
  }

  constexpr static inline real_t radians_to_degrees(real_t radians) {
    return radians * (180.f / pi);
  }

  static inline real_t satisfy_floating_point_tolerance(real_t value) {
    constexpr static real_t tolerance = 1e-4f;
    if (std::isnan(value) || std::isinf(value) || std::isnan(-value) || std::isinf(-value) ||
        glm::abs(value - 0.f) < tolerance) {
      return 0.f;
    }
    return value;
  }

}  // namespace other

#endif  // OTHER_CORE_MATH_CONSTANTS_HPP
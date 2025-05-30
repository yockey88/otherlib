/**
 * \file math/constants.hpp
 **/
#ifndef OTHER_MATH_CONSTANTS_HPP
#define OTHER_MATH_CONSTANTS_HPP

#include <limits>

namespace other {

  constexpr static float epsilon = std::numeric_limits<float>::epsilon();
  constexpr static float infinity = std::numeric_limits<float>::infinity();

  constexpr static float pi = 3.14159265358979323846f;
  constexpr static float two_pi = 2.f * pi;
  constexpr static float inv_pi = 1.f / pi;
  constexpr static float inv_two_pi = 1.f / two_pi;
  constexpr static float inv_four_pi = 1.f / (4.f * pi);

  constexpr static float degrees_to_radians(float degrees) {
    return degrees * (pi / 180.f);
  }

  constexpr static float radians_to_degrees(float radians) {
    return radians * (180.f / pi);
  }

}  // namespace other

#endif  // !OTHER_MATH_CONSTANTS_HPP
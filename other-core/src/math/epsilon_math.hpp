/**
 * \file math/epsilon_math.hpp
 **/
#ifndef OTHER_CORE_MATH_EPSILON_MATH_HPP
#define OTHER_CORE_MATH_EPSILON_MATH_HPP

#include "core/defines.hpp"

namespace other {
  namespace detail {

    constexpr real_t abs(real_t x) {
      return x < 0 ? -x : x;
    }

    constexpr real_t default_epsilon() {
      return 1e-4;
    }

    constexpr bool epsilon_zero(real_t x, real_t epsilon = default_epsilon()) {
      return detail::abs(x) < epsilon;
    }

    constexpr bool epsilon_equal(real_t a, real_t b, real_t epsilon = default_epsilon()) {
      return epsilon_zero(a - b, epsilon);
    }

    constexpr real_t epsilon_sum(real_t a, real_t b, real_t epsilon = default_epsilon()) {
      real_t sum = a + b;
      return epsilon_zero(sum) ? 0 : sum;
    }

    constexpr real_t epsilon_difference(real_t a, real_t b, real_t epsilon = default_epsilon()) {
      real_t diff = a - b;
      return epsilon_zero(diff) ? 0 : diff;
    }

    constexpr real_t epsilon_product(real_t a, real_t b, real_t epsilon = default_epsilon()) {
      real_t product = a * b;
      return epsilon_zero(product) ? 0 : product;
    }

    constexpr real_t epsilon_division(real_t a, real_t b, real_t epsilon = default_epsilon()) {
      if (epsilon_zero(b, epsilon)) {
        return 0;
      }
      return a / b;
    }

    constexpr bool epsilon_lt(real_t a, real_t b, real_t epsilon = default_epsilon()) {
      return epsilon_difference(a, b, epsilon) < 0;
    }

    constexpr bool epsilon_lte(real_t a, real_t b, real_t epsilon = default_epsilon()) {
      return epsilon_difference(a, b, epsilon) <= 0;
    }

    constexpr bool epsilon_gt(real_t a, real_t b, real_t epsilon = default_epsilon()) {
      return epsilon_difference(a, b, epsilon) > 0;
    }

    constexpr bool epsilon_gte(real_t a, real_t b, real_t epsilon = default_epsilon()) {
      return epsilon_difference(a, b, epsilon) >= 0;
    }

  }  // namespace detail
}  // namespace other

#endif  // OTHER_CORE_MATH_EPSILON_MATH_HPP
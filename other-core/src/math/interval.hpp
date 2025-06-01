/**
 * @file math/interval.hpp
 */
#ifndef OTHER_CORE_MATH_INTERVAL_HPP
#define OTHER_CORE_MATH_INTERVAL_HPP

#include <glm/glm.hpp>

#include "math/constants.hpp"

namespace other {

  struct interval {
    real_t min = +infinity;
    real_t max = -infinity;

    constexpr interval() = default;
    constexpr interval(real_t min, real_t max)
        : min(min), max(max) {}

    real_t size() const;

    bool contains(real_t t) const;
    bool contains(const interval& other) const;

    bool surrounds(real_t t) const;
    bool surrounds(const interval& other) const;

    bool overlaps(const interval& other) const;

    real_t clamp(real_t t) const;

    static interval infinite;
    static interval empty;
  };

}  // namespace other

#endif  // OTHER_CORE_MATH_INTERVAL_HPP
/**
 * \file math/interval.cpp
 **/
#include "math/interval.hpp"

namespace other {

  interval interval::infinite = { -std::numeric_limits<real_t>::max(), std::numeric_limits<real_t>::max() };
  interval interval::empty = { std::numeric_limits<real_t>::max(), -std::numeric_limits<real_t>::infinity() };

  real_t interval::size() const {
    return max - min;
  }

  bool interval::contains(real_t t) const {
    return (min <= t && t <= max);
  }

  bool interval::contains(const interval& other) const {
    return (min <= other.min && other.max <= max);
  }

  bool interval::surrounds(real_t t) const {
    return (min < t && t < max);
  }

  bool interval::surrounds(const interval& other) const {
    return (min < other.min && other.max < max);
  }

  bool interval::overlaps(const interval& other) const {
    return other.min <= max || other.max >= min;
  }

  real_t interval::clamp(real_t t) const {
    return t < min ? min : (t > max ? max : t);
  }

}  // namespace other
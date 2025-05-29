/**
 * \file math/interval.cpp
 **/
#include "math/interval.hpp"

namespace other {

  interval interval::infinite = { -std::numeric_limits<float>::max(), std::numeric_limits<float>::max() };
  interval interval::empty = { std::numeric_limits<float>::max(), -std::numeric_limits<float>::infinity() };

  float interval::size() const {
    return max - min;
  }

  bool interval::contains(float t) const {
    return (min <= t && t <= max);
  }

  bool interval::contains(const interval& other) const {
    return (min <= other.min && other.max <= max);
  }

  bool interval::surrounds(float t) const {
    return (min < t && t < max);
  }

  bool interval::surrounds(const interval& other) const {
    return (min < other.min && other.max < max);
  }

  bool interval::overlaps(const interval& other) const {
    return other.min <= max || other.max >= min;
  }

  float interval::clamp(float t) const {
    return t < min ? min : (t > max ? max : t);
  }

}  // namespace other
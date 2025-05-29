/**
 * \file math/interval.hpp
 **/
#ifndef OTHER_MATH_INTERVAL_HPP
#define OTHER_MATH_INTERVAL_HPP

#include <glm/glm.hpp>

#include "math/constants.hpp"

namespace other {

  struct interval {
    float min = +infinity;
    float max = -infinity;

    constexpr interval() = default;
    constexpr interval(float min, float max)
        : min(min), max(max) {}

    float size() const;

    bool contains(float t) const;
    bool contains(const interval& other) const;

    bool surrounds(float t) const;
    bool surrounds(const interval& other) const;

    bool overlaps(const interval& other) const;

    float clamp(float t) const;

    static interval infinite;
    static interval empty;
  };

}  // namespace other

#endif  // OTHER_MATH_INTERVAL_HPP
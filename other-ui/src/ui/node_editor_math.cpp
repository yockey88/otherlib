/**
 * \file ui/node_editor_math.cpp
 **/
#include "ui/node_editor_math.hpp"

namespace other {

  inline glm::vec2 cubic_at(float t, const glm::vec2& p0, const glm::vec2& c0, const glm::vec2& c1, const glm::vec2& p1) {
    const float u = 1.f - t;
    return u * u * u * p0 + 3.f * u * u * t * c0 + 3.f * u * t * t * c1 + t * t * t * p1;
  }

  float distance_to_segment(const glm::vec2& p, const glm::vec2& a, const glm::vec2& b) {
    const glm::vec2 ab = b - a;
    const float ab_length_squared = glm::dot(ab, ab);
    if (ab_length_squared == 0.f) {
      return glm::length(p - a);
    }

    const float t = glm::clamp(glm::dot(p - a, ab) / ab_length_squared, 0.f, 1.f);
    return glm::length(p - (a + t * ab));
  }

  float distance_to_cubic(const glm::vec2& p, const glm::vec2& p0, const glm::vec2& c0, const glm::vec2& c1, const glm::vec2& p1, float reject_beyond) {
    const glm::vec2 aabb_min = glm::min(glm::min(p0, c0), glm::min(c1, p1));
    const glm::vec2 aabb_max = glm::max(glm::max(p0, c0), glm::max(c1, p1));
    const float aabb_distance = glm::length(p - glm::clamp(p, aabb_min, aabb_max));
    if (aabb_distance > reject_beyond) {
      return aabb_distance;
    }

    constexpr uint32_t kCoarseSegments = 24;
    constexpr uint32_t kFineSegments = 8;
    float best = std::numeric_limits<float>::max();
    uint32_t best_chord = 1;
    glm::vec2 prev = p0;
    for (uint32_t i = 1; i <= kCoarseSegments; ++i) {
      const glm::vec2 pt = cubic_at(static_cast<float>(i) / kCoarseSegments, p0, c0, c1, p1);
      const float d = distance_to_segment(p, prev, pt);
      if (d < best) {
        best = d;
        best_chord = i;
      }
      prev = pt;
    }

    const float t_lo = glm::clamp((static_cast<float>(best_chord) - 2.f) / kCoarseSegments, 0.f, 1.f);
    const float t_hi = glm::clamp((static_cast<float>(best_chord) + 1.f) / kCoarseSegments, 0.f, 1.f);
    prev = cubic_at(t_lo, p0, c0, c1, p1);
    for (uint32_t i = 1; i <= kFineSegments; ++i) {
      const float t = t_lo + (t_hi - t_lo) * static_cast<float>(i) / kFineSegments;
      const glm::vec2 pt = cubic_at(t, p0, c0, c1, p1);
      best = glm::min(best, distance_to_segment(p, prev, pt));
      prev = pt;
    }
    return best;
  }

}  // namespace other
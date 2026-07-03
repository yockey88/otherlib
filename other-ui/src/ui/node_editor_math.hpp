/**
 * \file ui/node_editor_math.hpp
 **/
#ifndef OTHER_UI_NODE_EDITOR_MATH_HPP
#define OTHER_UI_NODE_EDITOR_MATH_HPP

#include <glm/glm.hpp>

namespace other {

  struct canvas_rect {
    glm::vec2 min = { 0.f, 0.f };
    glm::vec2 max = { 0.f, 0.f };

    bool contains(const glm::vec2& p) const {
      return p.x >= min.x && p.y >= min.y && p.x <= max.x && p.y <= max.y;
    }
    bool overlaps(const canvas_rect& o) const {
      return min.x <= o.max.x && o.min.x <= max.x && min.y <= o.max.y && o.min.y <= max.y;
    }
  };

  struct canvas_transform {
    glm::vec2 window_origin = { 0.f, 0.f };
    glm::vec2 pan = { 0.f, 0.f };
    float zoom = 1.f;

    inline glm::vec2 to_screen(const glm::vec2& canvas_pos) const { return window_origin + (canvas_pos - pan) * zoom; }
    inline glm::vec2 to_canvas(const glm::vec2& screen_pos) const { return (screen_pos - window_origin) / zoom + pan; }

    inline void zoom_around(const glm::vec2& screen_anchor, float new_zoom) {
      const glm::vec2 anchor_canvas = to_canvas(screen_anchor);
      zoom = new_zoom;
      pan = anchor_canvas - (screen_anchor - window_origin) / zoom;
    }
  };

  float distance_to_segment(const glm::vec2& p, const glm::vec2& a, const glm::vec2& b);
  float distance_to_cubic(const glm::vec2& p, const glm::vec2& p0, const glm::vec2& c0, const glm::vec2& c1, const glm::vec2& p1,
                          float reject_beyond = std::numeric_limits<float>::max());

}  // namespace other

#endif  // OTHER_UI_NODE_EDITOR_MATH_HPP
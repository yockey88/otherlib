/**
 * \file ui/ui_canvas.hpp
 **/
#ifndef OTHERLIB_RENDERER_UI_UI_CANVAS_HPP
#define OTHERLIB_RENDERER_UI_UI_CANVAS_HPP

#include <glm/glm.hpp>

namespace other {

  struct ui_canvas {
    float frame_padding = 0.f;

    glm::vec2 min_position{ 0.f, 0.f };
    glm::vec2 max_position{ 800.f, 600.f };

    glm::vec2 inner_min_positions() const {
      return glm::vec2{ min_position.x + frame_padding, min_position.y + frame_padding };
    }
    glm::vec2 inner_max_positions() const {
      return glm::vec2{ max_position.x - frame_padding, max_position.y - frame_padding };
    }
  };

}  // namespace other

#endif  // OTHERLIB_RENDERER_UI_UI_CANVAS_HPP
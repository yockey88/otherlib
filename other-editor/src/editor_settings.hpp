/**
 * \file src/editor_settings.hpp
 **/
#ifndef OTHER_EDITOR_EDITOR_SETTINGS_HPP
#define OTHER_EDITOR_EDITOR_SETTINGS_HPP

namespace other {

  struct editor_settings {
    float camera_move_speed = 0.1f;
    float camera_look_sensitivity = 0.1f;
    float grid_line_thickness = 1.f;
    float grid_granularity = 1.f;
  };

}  // namespace other

#endif  // OTHER_EDITOR_EDITOR_SETTINGS_HPP
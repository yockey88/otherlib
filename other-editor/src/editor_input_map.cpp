/**
 * \file editor_input_map.cpp
 **/
#include "editor_input_map.hpp"

namespace other {

  input_map get_default_editor_input_map() {
    input_map map;
    map.name = "editor-default";
    map.stick_dead_zone = 0.15f;
    map.trigger_dead_zone = 0.05f;

    {
      auto& ctx = map.add_context("global", /* transparent */ true);

      /// quit / close
      ctx.add_action("quit")
        .bind_key(key_code::Q, modifier_flags::CTRL);

      /// toggle fullscreen
      ctx.add_action("toggle_fullscreen")
        .bind_key(key_code::F11);
    }

    return map;
  }

}  // namespace other
/**
 * \file input/input_state.cpp
 **/
#include "input/input_state.hpp"

namespace other {

  void keyboard_state::clear() {
    keys.fill(false);
    active_modifiers = modifier_flags::NONE;
    text_input.clear();
  }

  void mouse_state::clear() {
    buttons.fill(false);
    position = { 0.f, 0.f };
    delta = { 0.f, 0.f };
    scroll = { 0.f, 0.f };
    relative_mode = false;
  }

  void single_gamepad_state::clear() {
    connected = false;
    instance_id = -1;
    type = gamepad_type::UNKNOWN;
    name.clear();
    buttons.fill(false);
    axes.fill(0.f);
    axes_processed.fill(0.f);
  }

  const single_gamepad_state* gamepad_state::primary() const {
    if (primary_index < 0 || primary_index >= static_cast<int32_t>(kMaxGamepads)) {
      return nullptr;
    }
    const auto& pad = pads[static_cast<size_t>(primary_index)];
    return pad.connected ? &pad : nullptr;
  }

  void gamepad_state::clear() {
    for (auto& p : pads) {
      p.clear();
    }
    primary_index = -1;
  }

  void input_frame_state::clear() {
    keyboard.clear();
    mouse.clear();
    gamepads.clear();
  }

}  // namespace other
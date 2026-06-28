/**
 * \file input/input_action.cpp
 **/
#include "input/input_action.hpp"

namespace other {

  input_action& input_action::bind(input_source src, float scale, uint8_t component) {
    bindings.push_back({ .source = src, .component = component, .scale = scale });
    return *this;
  }

  input_action& input_action::bind_key(key_code key, modifier_flags mods, float scale, uint8_t component) {
    return bind(key_source(key, mods), scale, component);
  }

  input_action& input_action::bind_gamepad_button(gamepad_button btn, float scale) {
    return bind(gamepad_btn_source(btn), scale, 0);
  }

  input_action& input_action::bind_gamepad_axis(gamepad_axis axis, float threshold, float scale, uint8_t component) {
    return bind(gamepad_axis_source(axis, threshold), scale, component);
  }

  input_action& input_action::bind_mouse_button(mouse_button btn, float scale) {
    return bind(mouse_btn_source(btn), scale, 0);
  }

  input_action& input_action::set_captures_text(bool v) {
    captures_text = v;
    return *this;
  }

  input_action& input_context::add_action(const std::string& action_name, action_value_type type, bool transparent) {
    actions.emplace_back(action_name, type, transparent);
    return actions.back();
  }

  input_action* input_context::find_action(natural_t action_id) {
    for (auto& a : actions) {
      if (a.id == action_id) {
        return &a;
      }
    }
    return nullptr;
  }

  const input_action* input_context::find_action(natural_t action_id) const {
    for (const auto& a : actions) {
      if (a.id == action_id) {
        return &a;
      }
    }
    return nullptr;
  }

  input_context& input_map::add_context(const std::string& ctx_name, bool transparent) {
    contexts.emplace_back(ctx_name, transparent);
    return contexts.back();
  }

  input_context* input_map::find_context(const std::string_view ctx_name) {
    natural_t id = FNV(ctx_name);
    for (auto& c : contexts) {
      if (c.id == id) {
        return &c;
      }
    }
    return nullptr;
  }

}  // namespace other
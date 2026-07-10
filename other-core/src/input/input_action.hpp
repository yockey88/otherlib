/**
 * \file input/input_action.hpp
 **/
#ifndef OTHER_CORE_INPUT_INPUT_ACTION_HPP
#define OTHER_CORE_INPUT_INPUT_ACTION_HPP

#include <functional>
#include <string>

#include "core/defines.hpp"
#include "core/fnv.hpp"
#include "data-structures/std_container.hpp"
#include "input/input_types.hpp"

namespace other {

  enum class action_value_type : uint8_t {
    DIGITAL = 0,
    AXIS_1D,
    AXIS_2D,
  };

  struct action_binding {
    input_source source{};

    uint8_t component = 0;
    float scale = 1.0f;
  };

  struct input_action {
    std::string name{};
    natural_t id = 0;  // FNV hash of name
    action_value_type value_type = action_value_type::DIGITAL;

    ostd::vector<action_binding> bindings{};

    /// whether this action should pass the raw text event to the consumer.
    /// if true, the action fires on any text input (used for console/chat).
    bool captures_text = false;
    bool transparent = true;

    input_action() = default;

    explicit input_action(const std::string& action_name, action_value_type type = action_value_type::DIGITAL, bool transparent = true)
        : name(action_name), id(FNV(action_name)), value_type(type), transparent(transparent) {}

    input_action& bind(input_source src, float scale = 1.f, uint8_t component = 0);
    input_action& bind_key(key_code key, modifier_flags mods = modifier_flags::NONE, float scale = 1.f, uint8_t component = 0);
    input_action& bind_gamepad_button(gamepad_button btn, float scale = 1.f);
    input_action& bind_gamepad_axis(gamepad_axis axis, float threshold = 0.5f, float scale = 1.f, uint8_t component = 0);
    input_action& bind_mouse_button(mouse_button btn, float scale = 1.f);
    input_action& set_captures_text(bool v = true);
  };

  struct action_state {
    float value = 0.f;
    float value_y = 0.f;
    bool just_pressed = false;
    bool just_released = false;
    bool active = false;
    std::string text{};
  };

  using context_transition_callback = std::function<void()>;

  struct input_context {
    std::string name{};
    natural_t id = 0;

    ostd::vector<input_action> actions{};

    /// if true, input that this context does not consume falls through to the next context on the stack
    /// if false, this context blocks all input from reaching lower contexts.
    bool transparent = false;

    context_transition_callback on_enter = nullptr;
    context_transition_callback on_exit = nullptr;

    input_context() = default;

    explicit input_context(const std::string& ctx_name, bool is_transparent = false)
        : name(ctx_name), id(FNV(ctx_name)), transparent(is_transparent) {}

    input_action& add_action(const std::string& action_name, action_value_type type = action_value_type::DIGITAL, bool transparent = true);
    input_action* find_action(natural_t action_id);
    const input_action* find_action(natural_t action_id) const;
  };

  /// this gets serialized as an input_map_asset.
  struct input_map {
    std::string name = "default";
    ostd::vector<input_context> contexts{};

    /// dead-zone settings for gamepad axes.
    float stick_dead_zone = 0.15f;
    float trigger_dead_zone = 0.05f;

    input_context& add_context(const std::string& ctx_name, bool transparent = false);
    input_context* find_context(const std::string_view ctx_name);
  };

}  // namespace other

#endif  // OTHER_CORE_INPUT_INPUT_ACTION_HPP
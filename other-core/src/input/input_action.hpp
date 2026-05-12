/**
 * \file input/input_action.hpp
 **/
#ifndef OTHER_CORE_INPUT_INPUT_ACTION_HPP
#define OTHER_CORE_INPUT_INPUT_ACTION_HPP

#include <functional>
#include <string>

#include "core/defines.hpp"
#include "core/fnv.hpp"
#include "input/input_types.hpp"

namespace other {

  enum class action_value_type : uint8_t {
    /// action either active (1.0) or inactive (0.0)
    DIGITAL = 0,

    /// action is single float in [-1, 1]
    //   for triggers, single-axis stick movement.
    AXIS_1D,

    /// action is vec2
    AXIS_2D,
  };

  /// physical-to-logical binding
  /// action can have many of these
  /// (keyboard W *and* gamepad left-stick-Y binded to "move_forward").
  struct action_binding {
    input_source source{};

    /// AXIS_1D / AXIS_2D actions, the contribution axis index
    //   0 = x or single, 1 = y
    //   ignored for DIGITAL.
    uint8_t component = 0;

    /// applied to the raw value before accumulation
    //   use -1.0 to invert
    float scale = 1.0f;
  };

  struct input_action {
    std::string name{};
    natural_t id = 0;  // FNV hash of name
    action_value_type value_type = action_value_type::DIGITAL;

    std::vector<action_binding> bindings{};

    /// whether this action should pass the raw text event to the consumer.
    /// if true, the action fires on any text input (used for console/chat).
    bool captures_text = false;

    input_action() = default;

    explicit input_action(const std::string& action_name, action_value_type type = action_value_type::DIGITAL)
        : name(action_name), id(FNV(action_name)), value_type(type) {}

    input_action& bind(input_source src, float scale = 1.f, uint8_t component = 0);
    input_action& bind_key(key_code key, modifier_flags mods = modifier_flags::NONE, float scale = 1.f, uint8_t component = 0);
    input_action& bind_gamepad_button(gamepad_button btn, float scale = 1.f);
    input_action& bind_gamepad_axis(gamepad_axis axis, float threshold = 0.5f, float scale = 1.f, uint8_t component = 0);
    input_action& bind_mouse_button(mouse_button btn, float scale = 1.f);
    input_action& set_captures_text(bool v = true);
  };

  struct action_state {
    /// DIGITAL: 1.0 when pressed, 0.0 otherwise.
    /// AXIS_1D value in [-1, 1].
    /// AXIS_2D x component.
    float value = 0.f;

    /// AXIS_2D y component.
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

    std::vector<input_action> actions{};

    /// if true, input that this context does not consume falls through to the next context on the stack
    /// if false, this context blocks all input from reaching lower contexts.
    bool transparent = false;

    context_transition_callback on_enter = nullptr;
    context_transition_callback on_exit = nullptr;

    input_context() = default;

    explicit input_context(const std::string& ctx_name, bool is_transparent = false)
        : name(ctx_name), id(FNV(ctx_name)), transparent(is_transparent) {}

    input_action& add_action(const std::string& action_name, action_value_type type = action_value_type::DIGITAL);
    input_action* find_action(natural_t action_id);
    const input_action* find_action(natural_t action_id) const;
  };

  /// this gets serialized as an input_map_asset.
  struct input_map {
    std::string name = "default";
    std::vector<input_context> contexts{};

    /// dead-zone settings for gamepad axes.
    float stick_dead_zone = 0.15f;
    float trigger_dead_zone = 0.05f;

    input_context& add_context(const std::string& ctx_name, bool transparent = false);
    input_context* find_context(const std::string_view ctx_name);
  };

}  // namespace other

#endif  // OTHER_CORE_INPUT_INPUT_ACTION_HPP
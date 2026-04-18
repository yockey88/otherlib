/**
 * \file input/input_state.hpp
 **/
#ifndef OTHER_CORE_INPUT_INPUT_STATE_HPP
#define OTHER_CORE_INPUT_INPUT_STATE_HPP

#include <array>
#include <string>

#include <glm/glm.hpp>

#include "input/input_types.hpp"

namespace other {

  struct keyboard_state {
    static constexpr size_t kNumKeys = static_cast<size_t>(key_code::NUM_KEY_CODES);

    std::array<bool, kNumKeys> keys{};
    modifier_flags active_modifiers = modifier_flags::NONE;

    std::string text_input{};

    inline bool is_down(key_code k) const { return keys[static_cast<size_t>(k)]; }

    void clear();
  };

  struct mouse_state {
    static constexpr size_t kNumButtons = static_cast<size_t>(mouse_button::NUM_MOUSE_BUTTONS);

    std::array<bool, kNumButtons> buttons{};

    glm::vec2 position = { 0.f, 0.f };
    glm::vec2 delta = { 0.f, 0.f };
    glm::vec2 scroll = { 0.f, 0.f };  // (x, y) – y is the typical scroll wheel

    /// True while relative mouse mode is active (pointer captured).
    bool relative_mode = false;

    inline bool is_down(mouse_button b) const { return buttons[static_cast<size_t>(b)]; }

    void clear();
  };

  static constexpr size_t kMaxGamepads = 4;

  enum class gamepad_type : uint8_t {
    UNKNOWN = 0,
    XBOX,
    PLAYSTATION,
    NINTENDO,
    GENERIC,
  };

  struct single_gamepad_state {
    static constexpr size_t kNumButtons = static_cast<size_t>(gamepad_button::NUM_GAMEPAD_BUTTONS);
    static constexpr size_t kNumAxes = static_cast<size_t>(gamepad_axis::NUM_GAMEPAD_AXES);

    bool connected = false;

    /// SDL joystick instance id, used for hot-plug tracking.
    int32_t instance_id = -1;

    gamepad_type type = gamepad_type::UNKNOWN;
    std::string name{};

    std::array<bool, kNumButtons> buttons{};
    std::array<float, kNumAxes> axes{};
    /// with deadzone
    std::array<float, kNumAxes> axes_processed{};

    inline bool is_down(gamepad_button b) const { return buttons[static_cast<size_t>(b)]; }
    inline float axis_value(gamepad_axis a) const { return axes_processed[static_cast<size_t>(a)]; }

    void clear();
  };

  struct gamepad_state {
    std::array<single_gamepad_state, kMaxGamepads> pads{};

    /// idx of "primary" gamepad (usually the first one connected).
    int32_t primary_index = -1;

    const single_gamepad_state* primary() const;
    void clear();
  };

  struct input_frame_state {
    keyboard_state keyboard{};
    mouse_state mouse{};
    gamepad_state gamepads{};

    void clear();
  };

}  // namespace other

#endif  // OTHER_CORE_INPUT_INPUT_STATE_HPP
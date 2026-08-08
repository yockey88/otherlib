/**
 * \file input/input_types.hpp
 **/
#ifndef OTHER_CORE_INPUT_INPUT_TYPES_HPP
#define OTHER_CORE_INPUT_INPUT_TYPES_HPP

#include <cstdint>

namespace other {

  enum class device_type : uint8_t {
    KEYBOARD = 0,
    MOUSE,
    GAMEPAD,

    NUM_DEVICE_TYPES,
  };

  /// logical key codes: the *meaning* of a key independent of physical layout (QWERTY
  ///  'W' is always KEY_W even on AZERTY); mapped 1:1 from SDL_Keycode at the boundary
  enum class key_code : uint16_t {
    UNKNOWN = 0,

    // letters
    A,
    B,
    C,
    D,
    E,
    F,
    G,
    H,
    I,
    J,
    K,
    L,
    M,
    N,
    O,
    P,
    Q,
    R,
    S,
    T,
    U,
    V,
    W,
    X,
    Y,
    Z,

    // digits (top row)
    NUM_0,
    NUM_1,
    NUM_2,
    NUM_3,
    NUM_4,
    NUM_5,
    NUM_6,
    NUM_7,
    NUM_8,
    NUM_9,

    // function keys
    F1,
    F2,
    F3,
    F4,
    F5,
    F6,
    F7,
    F8,
    F9,
    F10,
    F11,
    F12,

    // navigation
    ESCAPE,
    TAB,
    CAPS_LOCK,
    SPACE,
    ENTER,
    BACKSPACE,
    DEL,
    INSERT,
    HOME,
    END,
    PAGE_UP,
    PAGE_DOWN,
    ARROW_UP,
    ARROW_DOWN,
    ARROW_LEFT,
    ARROW_RIGHT,

    // modifiers
    LEFT_SHIFT,
    RIGHT_SHIFT,
    LEFT_CTRL,
    RIGHT_CTRL,
    LEFT_ALT,
    RIGHT_ALT,
    LEFT_SUPER,
    RIGHT_SUPER,

    // punctuation / symbols
    GRAVE_ACCENT,   // `~
    MINUS,          // -_
    EQUALS,         // =+
    LEFT_BRACKET,   // [{
    RIGHT_BRACKET,  // ]}
    BACKSLASH,      // \|
    SEMICOLON,      // ;:
    APOSTROPHE,     // '"
    COMMA,          // ,<
    PERIOD,         // .>
    SLASH,          // /?

    // numpad
    NUMPAD_0,
    NUMPAD_1,
    NUMPAD_2,
    NUMPAD_3,
    NUMPAD_4,
    NUMPAD_5,
    NUMPAD_6,
    NUMPAD_7,
    NUMPAD_8,
    NUMPAD_9,
    NUMPAD_ENTER,
    NUMPAD_PLUS,
    NUMPAD_MINUS,
    NUMPAD_MULTIPLY,
    NUMPAD_DIVIDE,
    NUMPAD_DECIMAL,
    NUM_LOCK,

    // misc
    PRINT_SCREEN,
    SCROLL_LOCK,
    PAUSE,
    MENU,

    NUM_KEY_CODES,
  };

  enum class modifier_flags : uint8_t {
    NONE = 0,
    SHIFT = 1 << 0,
    CTRL = 1 << 1,
    ALT = 1 << 2,
    SUPER = 1 << 3,
  };

  inline modifier_flags operator|(modifier_flags a, modifier_flags b) {
    return static_cast<modifier_flags>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
  }
  inline modifier_flags operator&(modifier_flags a, modifier_flags b) {
    return static_cast<modifier_flags>(static_cast<uint8_t>(a) & static_cast<uint8_t>(b));
  }
  inline bool has_flag(modifier_flags field, modifier_flags flag) {
    return (static_cast<uint8_t>(field) & static_cast<uint8_t>(flag)) != 0;
  }

  enum class mouse_button : uint8_t {
    LEFT = 0,
    RIGHT,
    MIDDLE,
    EXTRA_1,
    EXTRA_2,

    NUM_MOUSE_BUTTONS,
  };

  /// unified gamepad button enum (Xbox/PlayStation/generic); names are layout-neutral,
  ///  display name resolves at the UI layer based on controller type
  enum class gamepad_button : uint8_t {
    FACE_DOWN = 0,  // Xbox A  / PS Cross
    FACE_RIGHT,     // Xbox B  / PS Circle
    FACE_LEFT,      // Xbox X  / PS Square
    FACE_UP,        // Xbox Y  / PS Triangle

    LEFT_BUMPER,   // LB / L1
    RIGHT_BUMPER,  // RB / R1

    BACK,   // Back / Select / Share
    START,  // Start / Options
    GUIDE,  // Xbox / PS button

    LEFT_STICK,   // L3
    RIGHT_STICK,  // R3

    DPAD_UP,
    DPAD_DOWN,
    DPAD_LEFT,
    DPAD_RIGHT,

    // touchpad click (PS), misc button
    MISC_1,

    NUM_GAMEPAD_BUTTONS,
  };

  enum class gamepad_axis : uint8_t {
    LEFT_STICK_X = 0,
    LEFT_STICK_Y,
    RIGHT_STICK_X,
    RIGHT_STICK_Y,

    LEFT_TRIGGER,   // LT / L2
    RIGHT_TRIGGER,  // RT / R2

    NUM_GAMEPAD_AXES,
  };

  struct input_source {
    device_type device = device_type::KEYBOARD;

    union {
      key_code key = key_code::UNKNOWN;
      mouse_button mouse;
      gamepad_button gp_btn;
      gamepad_axis gp_axis;
    };

    /// For key combos that require modifiers (e.g. Shift+Semicolon = ':').
    modifier_flags required_modifiers = modifier_flags::NONE;

    /// For axis sources, the threshold at which the axis is considered "pressed".
    float axis_threshold = 0.5f;

    /// Whether this source is an axis (continuous value) or digital.
    bool is_axis = false;
  };

  constexpr static inline input_source key_source(key_code key, modifier_flags mods = modifier_flags::NONE) {
    input_source src{};
    src.device = device_type::KEYBOARD;
    src.key = key;
    src.required_modifiers = mods;
    return src;
  }

  constexpr static inline input_source mouse_btn_source(mouse_button btn) {
    input_source src{};
    src.device = device_type::MOUSE;
    src.mouse = btn;
    return src;
  }

  constexpr static inline input_source gamepad_btn_source(gamepad_button btn) {
    input_source src{};
    src.device = device_type::GAMEPAD;
    src.gp_btn = btn;
    return src;
  }

  constexpr static inline input_source gamepad_axis_source(gamepad_axis axis, float threshold = 0.5f) {
    input_source src{};
    src.device = device_type::GAMEPAD;
    src.gp_axis = axis;
    src.axis_threshold = threshold;
    src.is_axis = true;
    return src;
  }

}  // namespace other

#endif  // OTHER_CORE_INPUT_INPUT_TYPES_HPP
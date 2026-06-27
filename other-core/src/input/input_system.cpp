/**
 * \file input/input_system.cpp
 **/
#include "input/input_system.hpp"

#include <SDL3/SDL.h>
#include <imgui/imgui.h>

#include "core/enum_formatter.hpp"
#include "core/logger.hpp"

#include "SDL3/SDL_keycode.h"

namespace other {

  void input_system::initialize() {
    CORE_LOG_INFO("Initializing input system.");

    /// SDL_INIT_GAMEPAD implies SDL_INIT_JOYSTICK
    /// we do this in case the other environment has inputs configured for gamepad
    ///   even without a render backend loaded.
    {
      PROFILE_SECTION("input_system::initialize--sdl");
      if (!SDL_WasInit(SDL_INIT_GAMEPAD)) {
        if (!SDL_InitSubSystem(SDL_INIT_GAMEPAD)) {
          CORE_LOG_ERROR("Failed to initialize SDL gamepad subsystem: {}", SDL_GetError());
        }
      }
    }

    /// enumerate already-connected gamepads  plugged in before launch
    int count = 0;
    {
      PROFILE_SECTION("input_system::initialize--sdl-enumerate-gamepads");
      SDL_JoystickID* joysticks = SDL_GetGamepads(&count);
      if (joysticks) {
        for (int i = 0; i < count; ++i) {
          handle_gamepad_added(joysticks[i]);
        }
        SDL_free(joysticks);
      }
    }

    staging.clear();
    current.clear();
    previous.clear();

    CORE_LOG_INFO("Input system initialized. {} gamepad(s) detected.", count);
  }

  void input_system::shutdown() {
    for (auto& entry : sdl_gamepads) {
      if (entry.handle) {
        SDL_CloseGamepad(entry.handle);
      }
    }
    sdl_gamepads.clear();

    context_stack.clear();
    action_cache.clear();
    prev_action_cache.clear();
  }

  void input_system::process_event(const SDL_Event* event) {
    switch (event->type) {
      case SDL_EVENT_KEY_DOWN: {
        key_code kc = translate_sdl_keycode(event->key.key);
        if (kc != key_code::UNKNOWN) {
          staging.keyboard.keys[static_cast<size_t>(kc)] = true;
        }
        staging.keyboard.active_modifiers = translate_sdl_modifiers(event->key.mod);
      } break;

      case SDL_EVENT_KEY_UP: {
        key_code kc = translate_sdl_keycode(event->key.key);
        if (kc != key_code::UNKNOWN) {
          staging.keyboard.keys[static_cast<size_t>(kc)] = false;
        }
        staging.keyboard.active_modifiers = translate_sdl_modifiers(event->key.mod);
      } break;

      case SDL_EVENT_TEXT_INPUT:
        staging.keyboard.text_input += event->text.text;
        break;

      case SDL_EVENT_MOUSE_BUTTON_DOWN: {
        mouse_button mb = translate_sdl_mouse_button(event->button.button);
        if (mb != mouse_button::NUM_MOUSE_BUTTONS) {
          staging.mouse.buttons[static_cast<size_t>(mb)] = true;
        }
      } break;

      case SDL_EVENT_MOUSE_BUTTON_UP: {
        mouse_button mb = translate_sdl_mouse_button(event->button.button);
        if (mb != mouse_button::NUM_MOUSE_BUTTONS) {
          staging.mouse.buttons[static_cast<size_t>(mb)] = false;
        }
      } break;

      case SDL_EVENT_MOUSE_MOTION:
        staging.mouse.position = { event->motion.x, event->motion.y };
        staging.mouse.delta += glm::vec2{ event->motion.xrel, event->motion.yrel };
        break;
      case SDL_EVENT_MOUSE_WHEEL:
        staging.mouse.scroll += glm::vec2{ event->wheel.x, event->wheel.y };
        break;

      case SDL_EVENT_GAMEPAD_ADDED: handle_gamepad_added(event->gdevice.which); break;
      case SDL_EVENT_GAMEPAD_REMOVED: handle_gamepad_removed(event->gdevice.which); break;

      case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
        for (const auto& entry : sdl_gamepads) {
          if (entry.instance_id == event->gbutton.which && entry.our_index >= 0) {
            gamepad_button btn = translate_sdl_gamepad_button(static_cast<SDL_GamepadButton>(event->gbutton.button));
            if (btn != gamepad_button::NUM_GAMEPAD_BUTTONS) {
              staging.gamepads.pads[entry.our_index].buttons[static_cast<size_t>(btn)] = true;
            }
            break;
          }
        }
        break;

      case SDL_EVENT_GAMEPAD_BUTTON_UP:
        for (const auto& entry : sdl_gamepads) {
          if (entry.instance_id == event->gbutton.which && entry.our_index >= 0) {
            gamepad_button btn = translate_sdl_gamepad_button(static_cast<SDL_GamepadButton>(event->gbutton.button));
            if (btn != gamepad_button::NUM_GAMEPAD_BUTTONS) {
              staging.gamepads.pads[entry.our_index].buttons[static_cast<size_t>(btn)] = false;
            }
            break;
          }
        }
        break;

      case SDL_EVENT_GAMEPAD_AXIS_MOTION:
        for (const auto& entry : sdl_gamepads) {
          if (entry.instance_id == event->gaxis.which && entry.our_index >= 0) {
            gamepad_axis axis = translate_sdl_gamepad_axis(static_cast<SDL_GamepadAxis>(event->gaxis.axis));
            if (axis != gamepad_axis::NUM_GAMEPAD_AXES) {
              float normalized = static_cast<float>(event->gaxis.value) / 32767.f;
              staging.gamepads.pads[entry.our_index].axes[static_cast<size_t>(axis)] = normalized;
            }
            break;
          }
        }
        break;

      default:
        break;
    }
  }

  void input_system::update() {
    PROFILE_SECTION("input_system::update");

    previous = current;
    current = staging;

    /// apply dead-zones to gamepad axes
    for (auto& pad : current.gamepads.pads) {
      if (!pad.connected) continue;
      for (size_t i = 0; i < single_gamepad_state::kNumAxes; ++i) {
        float raw = pad.axes[i];
        bool is_trigger =
          static_cast<gamepad_axis>(i) == gamepad_axis::LEFT_TRIGGER ||
          static_cast<gamepad_axis>(i) == gamepad_axis::RIGHT_TRIGGER;
        float dz = is_trigger ?
          map.trigger_dead_zone :
          map.stick_dead_zone;
        pad.axes_processed[i] = apply_dead_zone(raw, dz);
      }
    }

    /// evaluate all actions in the active context stack

    {
      PROFILE_SECTION("input_system::update--evaluate_actions");
      evaluate_actions();
    }

    {
      PROFILE_SECTION("input_system::update--callbacks");
      for (auto& [id, state] : action_cache) {
        if (state.just_pressed || state.just_released) {
          input_state_change_event ev{
            .action_id = id,
            .pressed = state.just_pressed
          };

          for (const auto& ctx : map.contexts) {
            if (const auto* act = ctx.find_action(id)) {
              ev.action_name = act->name;
              break;
            }
          }
          for (const auto& cb : action_edge_callbacks) {
            cb(ev);
          }
        }
      }
    }
  }

  void input_system::finalize_frame() {
    PROFILE_SECTION("input_system::finalize_frame");

    staging.mouse.delta = { 0.f, 0.f };
    staging.mouse.scroll = { 0.f, 0.f };
    staging.keyboard.text_input.clear();

    prev_action_cache = action_cache;
    action_cache.clear();
  }

  void input_system::load_input_map(input_map&& map) {
    this->map = std::move(map);
    context_stack.clear();
    action_cache.clear();
    prev_action_cache.clear();

    CORE_LOG_INFO("Loaded input map '{}' with {} context(s).", this->map.name, this->map.contexts.size());
  }

  void input_system::push_context(const std::string_view name) {
    natural_t id = FNV(name);
    input_context* ctx = nullptr;
    for (auto& c : map.contexts) {
      if (c.id == id) {
        ctx = &c;
        break;
      }
    }
    if (!ctx) {
      CORE_LOG_ERROR("Cannot push unknown input context '{}'.", name);
      return;
    }

    context_stack.push_back(id);
    CORE_LOG_DEBUG("Pushed input context '{}'. Stack depth: {}.", name, context_stack.size());

    if (ctx->on_enter) {
      ctx->on_enter();
    }
  }

  void input_system::pop_context() {
    if (context_stack.empty()) {
      CORE_LOG_WARN("Attempted to pop from empty input context stack.");
      return;
    }

    natural_t id = context_stack.back();
    for (auto& c : map.contexts) {
      if (c.id == id && c.on_exit) {
        c.on_exit();
        break;
      }
    }

    context_stack.pop_back();
    CORE_LOG_DEBUG("Popped input context. Stack depth: {}.", context_stack.size());
  }

  void input_system::switch_context(const std::string_view name) {
    if (!context_stack.empty()) {
      pop_context();
    }
    push_context(name);
  }

  const input_context* input_system::active_context() const {
    if (context_stack.empty()) {
      return nullptr;
    }

    natural_t id = context_stack.back();
    for (const auto& c : map.contexts) {
      if (c.id == id) {
        return &c;
      }
    }
    return nullptr;
  }

  bool input_system::is_key_down(key_code key) const {
    return current.keyboard.is_down(key);
  }

  bool input_system::is_key_pressed(key_code key) const {
    return current.keyboard.is_down(key) && !previous.keyboard.is_down(key);
  }

  bool input_system::is_key_released(key_code key) const {
    return !current.keyboard.is_down(key) && previous.keyboard.is_down(key);
  }

  modifier_flags input_system::get_active_modifiers() const {
    return current.keyboard.active_modifiers;
  }

  const std::string& input_system::get_text_input() const {
    return current.keyboard.text_input;
  }

  bool input_system::is_mouse_down(mouse_button btn) const {
    return current.mouse.is_down(btn);
  }

  bool input_system::is_mouse_pressed(mouse_button btn) const {
    return current.mouse.is_down(btn) && !previous.mouse.is_down(btn);
  }

  bool input_system::is_mouse_released(mouse_button btn) const {
    return !current.mouse.is_down(btn) && previous.mouse.is_down(btn);
  }

  glm::vec2 input_system::get_mouse_position() const {
    return current.mouse.position;
  }

  glm::vec2 input_system::get_mouse_delta() const {
    return current.mouse.position - previous.mouse.position;
  }

  glm::vec2 input_system::get_mouse_scroll() const {
    return current.mouse.scroll;
  }

  void input_system::set_deadzone(gamepad_axis axis, float dead_zone, int32_t pad_index) {
    if (pad_index < 0 || pad_index >= static_cast<int32_t>(kMaxGamepads)) {
      CORE_LOG_WARN("Attempted to set gamepad deadzone for invalid pad index {}.", pad_index);
      return;
    }
    if (axis == gamepad_axis::LEFT_TRIGGER || axis == gamepad_axis::RIGHT_TRIGGER) {
      map.trigger_dead_zone = dead_zone;
    } else {
      map.stick_dead_zone = dead_zone;
    }
  }

  bool input_system::is_gamepad_connected(int32_t index) const {
    if (index < 0 || index >= static_cast<int32_t>(kMaxGamepads)) {
      return false;
    }
    return current.gamepads.pads[index].connected;
  }

  bool input_system::is_gamepad_button_down(gamepad_button btn, int32_t pad_index) const {
    if (pad_index < 0 || pad_index >= static_cast<int32_t>(kMaxGamepads)) {
      return false;
    }
    return current.gamepads.pads[pad_index].is_down(btn);
  }

  bool input_system::is_gamepad_button_pressed(gamepad_button btn, int32_t pad_index) const {
    if (pad_index < 0 || pad_index >= static_cast<int32_t>(kMaxGamepads)) {
      return false;
    }
    return current.gamepads.pads[pad_index].is_down(btn) && !previous.gamepads.pads[pad_index].is_down(btn);
  }

  bool input_system::is_gamepad_button_released(gamepad_button btn, int32_t pad_index) const {
    if (pad_index < 0 || pad_index >= static_cast<int32_t>(kMaxGamepads)) {
      return false;
    }
    return !current.gamepads.pads[pad_index].is_down(btn) && previous.gamepads.pads[pad_index].is_down(btn);
  }

  float input_system::get_gamepad_axis(gamepad_axis axis, int32_t pad_index) const {
    if (pad_index < 0 || pad_index >= static_cast<int32_t>(kMaxGamepads)) {
      return 0.f;
    }
    return current.gamepads.pads[pad_index].axis_value(axis);
  }

  gamepad_type input_system::get_gamepad_type(int32_t pad_index) const {
    if (pad_index < 0 || pad_index >= static_cast<int32_t>(kMaxGamepads)) {
      return gamepad_type::UNKNOWN;
    }
    return current.gamepads.pads[pad_index].type;
  }

  action_state input_system::get_action_state(const std::string_view action_name) const {
    return get_action_state(FNV(action_name));
  }

  action_state input_system::get_action_state(natural_t action_id) const {
    auto it = action_cache.find(action_id);
    if (it != action_cache.end()) {
      return it->second;
    }
    return action_state{};
  }

  bool input_system::is_action_pressed(const std::string_view name) const {
    return get_action_state(name).active;
  }

  bool input_system::is_action_just_pressed(const std::string_view name) const {
    return get_action_state(name).just_pressed;
  }

  bool input_system::is_action_just_released(const std::string_view name) const {
    return get_action_state(name).just_released;
  }

  float input_system::get_action_value(const std::string_view name) const {
    return get_action_state(name).value;
  }

  glm::vec2 input_system::get_action_value_2d(const std::string_view name) const {
    auto s = get_action_state(name);
    return { s.value, s.value_y };
  }

  void input_system::on_gamepad_connection(gamepad_connection_callback cb) {
    gamepad_connection_callbacks.push_back(std::move(cb));
  }

  void input_system::on_input_change_state(input_state_change_callback cb) {
    action_edge_callbacks.push_back(std::move(cb));
  }

  bool input_system::imgui_wants_keyboard() const {
    return ImGui::GetIO().WantCaptureKeyboard;
  }

  bool input_system::imgui_wants_mouse() const {
    return ImGui::GetIO().WantCaptureMouse;
  }

  key_code input_system::translate_sdl_keycode(uint32_t sdl_key) const {
    SDL_Keycode key = static_cast<SDL_Keycode>(sdl_key);

    /// SDL3 keycodes for letter keys are their ASCII values
    if (key >= SDLK_A && key <= SDLK_Z) {
      return static_cast<key_code>(static_cast<int>(key_code::A) + (key - SDLK_A));
    }
    if (key >= SDLK_0 && key <= SDLK_9) {
      return static_cast<key_code>(static_cast<int>(key_code::NUM_0) + (key - SDLK_0));
    }
    if (key >= SDLK_F1 && key <= SDLK_F12) {
      return static_cast<key_code>(static_cast<int>(key_code::F1) + (key - SDLK_F1));
    }

    switch (key) {
      case SDLK_ESCAPE: return key_code::ESCAPE;
      case SDLK_TAB: return key_code::TAB;
      case SDLK_CAPSLOCK: return key_code::CAPS_LOCK;
      case SDLK_SPACE: return key_code::SPACE;
      case SDLK_RETURN: return key_code::ENTER;
      case SDLK_BACKSPACE: return key_code::BACKSPACE;
      case SDLK_DELETE: return key_code::DEL;
      case SDLK_INSERT: return key_code::INSERT;
      case SDLK_HOME: return key_code::HOME;
      case SDLK_END: return key_code::END;
      case SDLK_PAGEUP: return key_code::PAGE_UP;
      case SDLK_PAGEDOWN: return key_code::PAGE_DOWN;
      case SDLK_UP: return key_code::ARROW_UP;
      case SDLK_DOWN: return key_code::ARROW_DOWN;
      case SDLK_LEFT: return key_code::ARROW_LEFT;
      case SDLK_RIGHT: return key_code::ARROW_RIGHT;
      case SDLK_LSHIFT: return key_code::LEFT_SHIFT;
      case SDLK_RSHIFT: return key_code::RIGHT_SHIFT;
      case SDLK_LCTRL: return key_code::LEFT_CTRL;
      case SDLK_RCTRL: return key_code::RIGHT_CTRL;
      case SDLK_LALT: return key_code::LEFT_ALT;
      case SDLK_RALT: return key_code::RIGHT_ALT;
      case SDLK_LGUI: return key_code::LEFT_SUPER;
      case SDLK_RGUI: return key_code::RIGHT_SUPER;
      case SDLK_GRAVE: return key_code::GRAVE_ACCENT;
      case SDLK_MINUS: return key_code::MINUS;
      case SDLK_EQUALS: return key_code::EQUALS;
      case SDLK_LEFTBRACKET: return key_code::LEFT_BRACKET;
      case SDLK_RIGHTBRACKET: return key_code::RIGHT_BRACKET;
      case SDLK_BACKSLASH: return key_code::BACKSLASH;
      case SDLK_SEMICOLON: return key_code::SEMICOLON;
      case SDLK_APOSTROPHE: return key_code::APOSTROPHE;
      case SDLK_COMMA: return key_code::COMMA;
      case SDLK_PERIOD: return key_code::PERIOD;
      case SDLK_SLASH: return key_code::SLASH;
      case SDLK_NUMLOCKCLEAR: return key_code::NUM_LOCK;
      case SDLK_PRINTSCREEN: return key_code::PRINT_SCREEN;
      case SDLK_SCROLLLOCK: return key_code::SCROLL_LOCK;
      case SDLK_PAUSE: return key_code::PAUSE;
      case SDLK_MENU: return key_code::MENU;
      default: return key_code::UNKNOWN;
    }
  }

  modifier_flags input_system::translate_sdl_modifiers(uint16_t sdl_mod) const {
    modifier_flags flags = modifier_flags::NONE;

    SDL_Keymod mod = static_cast<SDL_Keymod>(sdl_mod);
    if (mod & SDL_KMOD_SHIFT) {
      flags = flags | modifier_flags::SHIFT;
    }
    if (mod & SDL_KMOD_CTRL) {
      flags = flags | modifier_flags::CTRL;
    }
    if (mod & SDL_KMOD_ALT) {
      flags = flags | modifier_flags::ALT;
    }
    if (mod & SDL_KMOD_GUI) {
      flags = flags | modifier_flags::SUPER;
    }
    return flags;
  }

  mouse_button input_system::translate_sdl_mouse_button(uint8_t sdl_button) const {
    switch (sdl_button) {
      case SDL_BUTTON_LEFT: return mouse_button::LEFT;
      case SDL_BUTTON_RIGHT: return mouse_button::RIGHT;
      case SDL_BUTTON_MIDDLE: return mouse_button::MIDDLE;
      case SDL_BUTTON_X1: return mouse_button::EXTRA_1;
      case SDL_BUTTON_X2: return mouse_button::EXTRA_2;
      default: return mouse_button::NUM_MOUSE_BUTTONS;
    }
  }

  gamepad_button input_system::translate_sdl_gamepad_button(SDL_GamepadButton sdl_btn) const {
    switch (sdl_btn) {
      case SDL_GAMEPAD_BUTTON_SOUTH: return gamepad_button::FACE_DOWN;
      case SDL_GAMEPAD_BUTTON_EAST: return gamepad_button::FACE_RIGHT;
      case SDL_GAMEPAD_BUTTON_WEST: return gamepad_button::FACE_LEFT;
      case SDL_GAMEPAD_BUTTON_NORTH: return gamepad_button::FACE_UP;
      case SDL_GAMEPAD_BUTTON_LEFT_SHOULDER: return gamepad_button::LEFT_BUMPER;
      case SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER: return gamepad_button::RIGHT_BUMPER;
      case SDL_GAMEPAD_BUTTON_BACK: return gamepad_button::BACK;
      case SDL_GAMEPAD_BUTTON_START: return gamepad_button::START;
      case SDL_GAMEPAD_BUTTON_GUIDE: return gamepad_button::GUIDE;
      case SDL_GAMEPAD_BUTTON_LEFT_STICK: return gamepad_button::LEFT_STICK;
      case SDL_GAMEPAD_BUTTON_RIGHT_STICK: return gamepad_button::RIGHT_STICK;
      case SDL_GAMEPAD_BUTTON_DPAD_UP: return gamepad_button::DPAD_UP;
      case SDL_GAMEPAD_BUTTON_DPAD_DOWN: return gamepad_button::DPAD_DOWN;
      case SDL_GAMEPAD_BUTTON_DPAD_LEFT: return gamepad_button::DPAD_LEFT;
      case SDL_GAMEPAD_BUTTON_DPAD_RIGHT: return gamepad_button::DPAD_RIGHT;
      case SDL_GAMEPAD_BUTTON_MISC1: return gamepad_button::MISC_1;
      default: return gamepad_button::NUM_GAMEPAD_BUTTONS;
    }
  }

  gamepad_axis input_system::translate_sdl_gamepad_axis(SDL_GamepadAxis sdl_axis) const {
    switch (sdl_axis) {
      case SDL_GAMEPAD_AXIS_LEFTX: return gamepad_axis::LEFT_STICK_X;
      case SDL_GAMEPAD_AXIS_LEFTY: return gamepad_axis::LEFT_STICK_Y;
      case SDL_GAMEPAD_AXIS_RIGHTX: return gamepad_axis::RIGHT_STICK_X;
      case SDL_GAMEPAD_AXIS_RIGHTY: return gamepad_axis::RIGHT_STICK_Y;
      case SDL_GAMEPAD_AXIS_LEFT_TRIGGER: return gamepad_axis::LEFT_TRIGGER;
      case SDL_GAMEPAD_AXIS_RIGHT_TRIGGER: return gamepad_axis::RIGHT_TRIGGER;
      default: return gamepad_axis::NUM_GAMEPAD_AXES;
    }
  }

  void input_system::handle_gamepad_added(uint32_t id) {
    SDL_JoystickID sdl_id = static_cast<SDL_JoystickID>(id);

    int32_t slot = -1;
    for (size_t i = 0; i < kMaxGamepads; ++i) {
      if (!staging.gamepads.pads[i].connected) {
        slot = static_cast<int32_t>(i);
        break;
      }
    }
    if (slot < 0) {
      CORE_LOG_WARN("Maximum gamepads ({}) already connected. Ignoring new device.", kMaxGamepads);
      return;
    }

    SDL_Gamepad* pad = SDL_OpenGamepad(sdl_id);
    if (!pad) {
      CORE_LOG_ERROR("Failed to open gamepad (instance {}): {}", id, SDL_GetError());
      return;
    }

    sdl_gamepad_entry entry;
    entry.handle = pad;
    entry.instance_id = id;
    entry.our_index = slot;
    sdl_gamepads.push_back(entry);

    auto& state = staging.gamepads.pads[slot];
    state.connected = true;
    state.instance_id = id;
    state.type = detect_gamepad_type(pad);
    const char* name = SDL_GetGamepadName(pad);
    state.name = name ? name : "Unknown Gamepad";

    if (staging.gamepads.primary_index < 0) {
      staging.gamepads.primary_index = slot;
    }

    CORE_LOG_INFO("Gamepad connected: '{}' (slot {}, type {}).", state.name, slot, state.type);

    gamepad_connection_event ev;
    ev.pad_index = slot;
    ev.connected = true;
    ev.instance_id = id;
    ev.type = state.type;
    ev.name = state.name;
    for (const auto& cb : gamepad_connection_callbacks) {
      cb(ev);
    }
  }

  void input_system::handle_gamepad_removed(uint32_t id) {
    for (auto it = sdl_gamepads.begin(); it != sdl_gamepads.end(); ++it) {
      if (it->instance_id == id) {
        int32_t slot = it->our_index;

        CORE_LOG_INFO("Gamepad disconnected: slot {}.", slot);

        if (it->handle) {
          SDL_CloseGamepad(it->handle);
        }

        if (slot >= 0 && slot < static_cast<int32_t>(kMaxGamepads)) {
          staging.gamepads.pads[slot].clear();
        }

        /// reassign primary if needed
        if (staging.gamepads.primary_index == slot) {
          staging.gamepads.primary_index = -1;
          for (size_t i = 0; i < kMaxGamepads; ++i) {
            if (staging.gamepads.pads[i].connected) {
              staging.gamepads.primary_index = static_cast<int32_t>(i);
              break;
            }
          }
        }

        gamepad_connection_event ev;
        ev.pad_index = slot;
        ev.connected = false;
        ev.instance_id = id;
        // ev.type = it->type;
        // ev.name = it->name;
        for (const auto& cb : gamepad_connection_callbacks) {
          cb(ev);
        }

        sdl_gamepads.erase(it);
        return;
      }
    }
  }

  gamepad_type input_system::detect_gamepad_type(SDL_Gamepad* pad) const {
    OTHER_ASSERT(pad != nullptr, "Gamepad pointer is null");

    SDL_GamepadType sdl_type = SDL_GetGamepadType(pad);
    switch (sdl_type) {
      case SDL_GAMEPAD_TYPE_XBOX360:
      case SDL_GAMEPAD_TYPE_XBOXONE:
        return gamepad_type::XBOX;
      case SDL_GAMEPAD_TYPE_PS3:
      case SDL_GAMEPAD_TYPE_PS4:
      case SDL_GAMEPAD_TYPE_PS5:
        return gamepad_type::PLAYSTATION;
      case SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_PRO:
      case SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_JOYCON_LEFT:
      case SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_JOYCON_RIGHT:
      case SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_JOYCON_PAIR:
        return gamepad_type::NINTENDO;
      default:
        return gamepad_type::GENERIC;
    }
  }

  float input_system::apply_dead_zone(float raw, float dead_zone) const {
    if (glm::abs(raw) < dead_zone) {
      return 0.f;
    }

    /// rescale so the usable range is [0, 1] after the dead zone
    float sign = raw > 0.f ?
      1.f :
      -1.f;
    return sign * (glm::abs(raw) - dead_zone) / (1.f - dead_zone);
  }

  void input_system::evaluate_actions() {
    /// walk the context stack top-down
    bool continue_evaluating = true;
    for (auto it = context_stack.rbegin(); it != context_stack.rend() && continue_evaluating; ++it) {
      natural_t ctx_id = *it;
      const input_context* ctx = nullptr;
      for (const auto& c : map.contexts) {
        if (c.id == ctx_id) {
          ctx = &c;
          break;
        }
      }
      if (!ctx) {
        continue;
      }

      continue_evaluating = ctx->transparent;
      for (const auto& action : ctx->actions) {
        /// skip if already evaluated by a higher context
        if (action_cache.contains(action.id)) {
          continue;
        }

        action_state state{};

        float accum_x = 0.f;
        float accum_y = 0.f;

        for (const auto& binding : action.bindings) {
          float raw = evaluate_binding(binding);

          if (action.value_type == action_value_type::DIGITAL) {
            if (raw != 0.f) {
              accum_x = 1.f;
            }
          } else if (action.value_type == action_value_type::AXIS_1D) {
            accum_x += raw * binding.scale;
          } else if (action.value_type == action_value_type::AXIS_2D) {
            if (binding.component == 0) {
              accum_x += raw * binding.scale;
            } else {
              accum_y += raw * binding.scale;
            }
          }
        }

        state.value = glm::clamp(accum_x, -1.f, 1.f);
        state.value_y = glm::clamp(accum_y, -1.f, 1.f);

        bool was_active = false;
        auto prev_it = prev_action_cache.find(action.id);
        if (prev_it != prev_action_cache.end()) {
          was_active = prev_it->second.active;
        }

        if (action.value_type == action_value_type::DIGITAL) {
          state.active = state.value != 0.f;
        } else {
          float mag = glm::length(glm::vec2(state.value, state.value_y));
          state.active = mag > 0.01f;
        }

        state.just_pressed = state.active && !was_active;
        state.just_released = !state.active && was_active;

        action_cache[action.id] = state;

        // if activated and opaque, then don't continue
        continue_evaluating = continue_evaluating && !state.active;
      }

      /// if this context is opaque, stop walking further down
      if (!ctx->transparent) {
        break;
      }
    }
  }

  float input_system::evaluate_binding(const action_binding& binding) const {
    const auto& src = binding.source;

    switch (src.device) {
      case device_type::KEYBOARD: {
        /// check modifier requirements
        if (src.required_modifiers != modifier_flags::NONE) {
          if ((current.keyboard.active_modifiers & src.required_modifiers) != src.required_modifiers) {
            return 0.f;
          }
        }
        return current.keyboard.is_down(src.key) ? 1.f : 0.f;
      }

      case device_type::MOUSE: {
        if (src.is_axis) {
          /// mouse axes aren't really used here – mouse delta is handled
          /// through the raw state API. But for completeness:
          return 0.f;
        }
        return current.mouse.is_down(src.mouse) ? 1.f : 0.f;
      }

      case device_type::GAMEPAD: {
        const auto* pad = current.gamepads.primary();
        if (!pad) {
          return 0.f;
        }

        if (src.is_axis) {
          return pad->axis_value(src.gp_axis);
        }
        return pad->is_down(src.gp_btn) ? 1.f : 0.f;
      }

      default:
        return 0.f;
    }
  }

}  // namespace other
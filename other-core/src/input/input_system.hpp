/**
 * \file input/input_system.hpp
 **/
#ifndef OTHER_CORE_INPUT_INPUT_SYSTEM_HPP
#define OTHER_CORE_INPUT_INPUT_SYSTEM_HPP

#include <cstdint>

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_gamepad.h>

#include "core/subsystem.hpp"
#include "input/input_action.hpp"
#include "input/input_state.hpp"
#include "input/input_types.hpp"

struct SDL_Gamepad;

namespace other {

  /// Fired when a gamepad is connected or disconnected.
  struct gamepad_connection_event {
    int32_t pad_index = -1;
    bool connected = false;
    uint32_t instance_id = 0;
    gamepad_type type = gamepad_type::UNKNOWN;
    std::string name{};
  };
  using gamepad_connection_callback = std::function<void(const gamepad_connection_event&)>;

  /// triggered by input system when context-level action is 'just_pressed' or 'just_released'. Useful for one-shot reactions (e.g. `:` opening the console).
  struct input_state_change_event {
    natural_t action_id = 0;
    std::string action_name{};
    // true = just pressed, false = just released
    bool pressed = false;
  };
  using input_state_change_callback = std::function<void(const input_state_change_event&)>;

  class OTHER_CLASS input_system : public subsystem<input_system> {
   public:
    virtual ~input_system() = default;

    void initialize();
    void shutdown();

    void process_event(const SDL_Event* event);

    void update();
    void finalize_frame();

    void load_input_map(input_map&& map);

    void push_context(const std::string_view name);
    void pop_context();

    /// replace the topmost context
    /// equivalent to pop + push but avoids a frame with no context.
    void switch_context(const std::string_view name);

    const input_context* active_context() const;

    bool is_key_down(key_code key) const;
    bool is_key_pressed(key_code key) const;   // was up, now down
    bool is_key_released(key_code key) const;  // was down, now up
    modifier_flags get_active_modifiers() const;

    const std::string& get_text_input() const;

    bool is_mouse_down(mouse_button btn) const;
    bool is_mouse_pressed(mouse_button btn) const;
    bool is_mouse_released(mouse_button btn) const;
    glm::vec2 get_mouse_position() const;
    glm::vec2 get_mouse_delta() const;
    glm::vec2 get_mouse_scroll() const;

    void set_deadzone(gamepad_axis axis, float dead_zone, int32_t pad_index = 0);
    bool is_gamepad_connected(int32_t index = 0) const;
    bool is_gamepad_button_down(gamepad_button btn, int32_t pad_index = 0) const;
    bool is_gamepad_button_pressed(gamepad_button btn, int32_t pad_index = 0) const;
    bool is_gamepad_button_released(gamepad_button btn, int32_t pad_index = 0) const;
    float get_gamepad_axis(gamepad_axis axis, int32_t pad_index = 0) const;
    gamepad_type get_gamepad_type(int32_t pad_index = 0) const;

    action_state get_action_state(const std::string_view action_name) const;
    action_state get_action_state(natural_t action_id) const;

    bool is_action_pressed(const std::string_view name) const;
    bool is_action_just_pressed(const std::string_view name) const;
    bool is_action_just_released(const std::string_view name) const;

    float get_action_value(const std::string_view name) const;
    glm::vec2 get_action_value_2d(const std::string_view name) const;

    void on_gamepad_connection(gamepad_connection_callback cb);
    void on_input_change_state(input_state_change_callback cb);

    bool imgui_wants_keyboard() const;
    bool imgui_wants_mouse() const;

    inline const input_frame_state& current_state() const { return current; }
    inline const input_frame_state& previous_state() const { return previous; }
    inline const input_map& get_input_map() const { return map; }

   private:
    input_frame_state staging;  // written during process_event
    input_frame_state current;  // read-only during the frame
    input_frame_state previous;

    std::unordered_map<natural_t, action_state> action_cache;
    std::unordered_map<natural_t, action_state> prev_action_cache;

    input_map map;

    /// back = top (active)
    std::vector<natural_t> context_stack;

    struct sdl_gamepad_entry {
      SDL_Gamepad* handle = nullptr;
      uint32_t instance_id = 0;
      int32_t our_index = -1;
    };
    std::vector<sdl_gamepad_entry> sdl_gamepads;

    std::vector<gamepad_connection_callback> gamepad_connection_callbacks;
    std::vector<input_state_change_callback> action_edge_callbacks;

    key_code translate_sdl_keycode(uint32_t sdl_key) const;
    modifier_flags translate_sdl_modifiers(uint16_t sdl_mod) const;
    mouse_button translate_sdl_mouse_button(uint8_t sdl_button) const;
    gamepad_button translate_sdl_gamepad_button(SDL_GamepadButton sdl_btn) const;
    gamepad_axis translate_sdl_gamepad_axis(SDL_GamepadAxis sdl_axis) const;

    void handle_gamepad_added(uint32_t id);
    void handle_gamepad_removed(uint32_t id);
    gamepad_type detect_gamepad_type(SDL_Gamepad* pad) const;

    float apply_dead_zone(float raw, float dead_zone) const;
    void evaluate_actions();
    float evaluate_binding(const action_binding& binding) const;
  };

}  // namespace other

OTHER_DEPENDENT_SUBSYSTEM(
  other::input_system,
  subsystem_profile::kArena,
  subsystem_profile::kLogger);

#endif  // OTHER_CORE_INPUT_INPUT_SYSTEM_HPP
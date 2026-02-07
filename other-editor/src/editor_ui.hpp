/**
 * \file editor_ui.hpp
 **/
#ifndef OTHER_EDITOR_SRC_EDITOR_UI_HPP
#define OTHER_EDITOR_SRC_EDITOR_UI_HPP

#include "core/scope.hpp"
#include "core/state_machine.hpp"
#include "event/event_system.hpp"

#include "ui/console.hpp"

namespace other {

  enum class editor_ui_state {
    EDITOR_UI_STATE_HIDDEN,
    EDITOR_UI_STATE_CONSOLE,

    NUM_STATES,
  };

  enum class editor_ui_event {
    EDITOR_UI_EVENT_OPEN_CONSOLE,
    EDITOR_UI_EVENT_CLOSE_CONSOLE,

    NUM_EVENTS
  };

  class editor_ui_state_machine : public state_machine<editor_ui_state, editor_ui_event> {
   public:
    editor_ui_state_machine()
        : state_machine<editor_ui_state, editor_ui_event>(editor_ui_state::EDITOR_UI_STATE_CONSOLE) {
      add_transition(editor_ui_state::EDITOR_UI_STATE_CONSOLE, editor_ui_event::EDITOR_UI_EVENT_CLOSE_CONSOLE, editor_ui_state::EDITOR_UI_STATE_HIDDEN);

      add_transition(editor_ui_state::EDITOR_UI_STATE_HIDDEN, editor_ui_event::EDITOR_UI_EVENT_OPEN_CONSOLE, editor_ui_state::EDITOR_UI_STATE_CONSOLE);
    }
  };

  class editor_ui {
   public:
    editor_ui(scope<event_system>& events, driver* driver_ptr)
        : event_system(events), driver_ptr(driver_ptr) {}
    ~editor_ui() = default;

    void initialize();
    void render();
    void shutdown();

   private:
    editor_ui_state_machine ui_state_machine;
    scope<event_system>& event_system;

    driver* driver_ptr;
    scope<ui_window> console_window_ptr = nullptr;
  };

}  // namespace other

#endif  // OTHER_EDITOR_SRC_EDITOR_UI_HPP
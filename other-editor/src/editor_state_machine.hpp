/**
 * \file src/editor_state_machine.hpp
 **/
#include "core/state_machine.hpp"

namespace other {

  enum class editor_state {
    EDITOR_STATE_STOPPED,
    EDITOR_STATE_INITIALIZING,
    EDITOR_STATE_RUNNING,
    EDITOR_STATE_SHUTTING_DOWN,
    NUM_STATES,
  };

  enum class editor_event {
    EDITOR_EVENT_START,
    EDITOR_EVENT_READY,
    EDITOR_EVENT_STOP,

    NUM_EVENTS,
  };

  class editor_state_machine : public state_machine<editor_state, editor_event> {
   public:
    editor_state_machine()
        : state_machine<editor_state, editor_event>(editor_state::EDITOR_STATE_STOPPED) {
      add_transition(editor_state::EDITOR_STATE_STOPPED, editor_event::EDITOR_EVENT_START, editor_state::EDITOR_STATE_INITIALIZING);

      add_transition(editor_state::EDITOR_STATE_INITIALIZING, editor_event::EDITOR_EVENT_READY, editor_state::EDITOR_STATE_RUNNING);

      add_transition(editor_state::EDITOR_STATE_RUNNING, editor_event::EDITOR_EVENT_STOP, editor_state::EDITOR_STATE_SHUTTING_DOWN);

      add_transition(editor_state::EDITOR_STATE_SHUTTING_DOWN, editor_event::EDITOR_EVENT_READY, editor_state::EDITOR_STATE_STOPPED);
    }
  };

}  // namespace other
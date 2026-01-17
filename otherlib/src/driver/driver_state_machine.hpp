/**
 * \file driver/driver_state_machine.hpp
 **/
#ifndef OTHERLIB_DRIVER_DRIVER_STATE_MACHINE_HPP
#define OTHERLIB_DRIVER_DRIVER_STATE_MACHINE_HPP

#include "core/state_machine.hpp"

namespace other {

  enum class driver_state {
    DRIVER_STATE_STOPPED,
    DRIVER_STATE_INITIALIZING,
    DRIVER_STATE_RUNNING,
    DRIVER_STATE_PAUSED,
    DRIVER_STATE_SHUTTING_DOWN,
    NUM_STATES,
  };

  enum class driver_event {
    DRIVER_EVENT_START,
    DRIVER_EVENT_READY,
    DRIVER_EVENT_PAUSE,
    DRIVER_EVENT_RESUME,
    DRIVER_EVENT_STOP,

    NUM_EVENTS,
  };

  class driver_state_machine : public state_machine<driver_state, driver_event> {
   public:
    driver_state_machine()
        : state_machine<driver_state, driver_event>(driver_state::DRIVER_STATE_STOPPED) {
      add_transition(driver_state::DRIVER_STATE_STOPPED, driver_event::DRIVER_EVENT_START, driver_state::DRIVER_STATE_INITIALIZING);

      add_transition(driver_state::DRIVER_STATE_INITIALIZING, driver_event::DRIVER_EVENT_READY, driver_state::DRIVER_STATE_RUNNING);

      add_transition(driver_state::DRIVER_STATE_RUNNING, driver_event::DRIVER_EVENT_STOP, driver_state::DRIVER_STATE_SHUTTING_DOWN);
      add_transition(driver_state::DRIVER_STATE_RUNNING, driver_event::DRIVER_EVENT_PAUSE, driver_state::DRIVER_STATE_PAUSED);

      add_transition(driver_state::DRIVER_STATE_PAUSED, driver_event::DRIVER_EVENT_STOP, driver_state::DRIVER_STATE_SHUTTING_DOWN);
      add_transition(driver_state::DRIVER_STATE_PAUSED, driver_event::DRIVER_EVENT_RESUME, driver_state::DRIVER_STATE_RUNNING);

      add_transition(driver_state::DRIVER_STATE_SHUTTING_DOWN, driver_event::DRIVER_EVENT_READY, driver_state::DRIVER_STATE_STOPPED);
    }
  };

}  // namespace other

#endif  // OTHER_DRIVER_DRIVER_STATE_MACHINE_HPP
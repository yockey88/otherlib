/**
 * \file network/session_state_machine.hpp
 **/
#ifndef DEVELOPMENT_DRIVERS_SERVER_DEV_SERVER_STATE_MACHINE_HPP
#define DEVELOPMENT_DRIVERS_SERVER_DEV_SERVER_STATE_MACHINE_HPP

#include "core/state_machine.hpp"

namespace other {
  namespace network {

    enum session_state {
      SESSION_STATE_STOPPED,
      SESSION_STATE_LAUNCHING,
      SESSION_STATE_STARTED,
      SESSION_STATE_SHUTTING_DOWN,

      NUM_STATES,
      INVALID_STATE = NUM_STATES
    };

    enum session_event {
      SESSION_EVENT_START,
      SESSION_EVENT_CHECK_IN,
      SESSION_EVENT_STATUS_CHECK,
      SESSION_EVENT_SHUTDOWN_START,
      SESSION_EVENT_SHUTDOWN_COMPLETE,

      NUM_EVENTS,
      INVALID_EVENT = NUM_EVENTS
    };

  }  // namespace network

  class session_state_machine : public state_machine<network::session_state, network::session_event> {
   public:
    session_state_machine()
        : state_machine<network::session_state, network::session_event>(network::SESSION_STATE_STOPPED) {
      add_transition(network::SESSION_STATE_STOPPED, network::SESSION_EVENT_START, network::SESSION_STATE_LAUNCHING);
      add_transition(network::SESSION_STATE_LAUNCHING, network::SESSION_EVENT_CHECK_IN, network::SESSION_STATE_STARTED /* ,  respond_to_check_in*/);
      add_transition(network::SESSION_STATE_STARTED, network::SESSION_EVENT_SHUTDOWN_START, network::SESSION_STATE_SHUTTING_DOWN);
      add_transition(network::SESSION_STATE_SHUTTING_DOWN, network::SESSION_EVENT_SHUTDOWN_COMPLETE, network::SESSION_STATE_STOPPED);
    }
    virtual ~session_state_machine() = default;
  };

}  // namespace other

#endif  // DEVELOPMENT_DRIVERS_SERVER_DEV_SERVER_STATE_MACHINE_HPP
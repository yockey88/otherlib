/**
 * \file network/connect_state_machine.hpp
 **/
#ifndef OTHERLIB_NETWORK_TCP_CONNECTION_STATE_MACHING_HPP
#define OTHERLIB_NETWORK_TCP_CONNECTION_STATE_MACHING_HPP

#include "core/state_machine.hpp"

namespace other {

  enum class connection_state {
    DISCONNECTED,
    CONNECTING,
    RECONNECTING,
    CONNECTED,
    DISCONNECTING,

    NUM_STATES,
  };

  enum class connection_event {
    START_CONNECT,
    CONNECT_SUCCESS,
    CONNECT_FAILURE_NO_RETRY,
    CONNECT_FAILURE_RETRY,

    SEND_SUCCESS,
    SEND_FAILURE_NO_RETRY,
    SEND_FAILURE_RETRY,

    CONNECTION_LOST_NO_RETRY,
    CONNECTION_LOST_RETRY,

    READ_SUCCESS,
    READ_FAILURE,

    START_DISCONNECT,
    DISCONNECT_SUCCESS,
    DISCONNECT_FAILURE,

    NUM_EVENTS,
  };

  class connection_state_machine : public state_machine<connection_state, connection_event> {
   public:
    connection_state_machine()
        : state_machine(connection_state::DISCONNECTED) {
      add_transition(connection_state::DISCONNECTED, connection_event::START_CONNECT, connection_state::CONNECTING);
      add_transition(connection_state::DISCONNECTED, connection_event::CONNECT_SUCCESS, connection_state::CONNECTED);

      add_transition(connection_state::CONNECTING, connection_event::CONNECT_SUCCESS, connection_state::CONNECTED);
      add_transition(connection_state::CONNECTING, connection_event::CONNECT_FAILURE_NO_RETRY, connection_state::DISCONNECTED);
      add_transition(connection_state::CONNECTING, connection_event::CONNECT_FAILURE_RETRY, connection_state::RECONNECTING);
      add_transition(connection_state::CONNECTING, connection_event::SEND_SUCCESS, connection_state::DISCONNECTED);
      add_transition(connection_state::CONNECTING, connection_event::SEND_FAILURE_NO_RETRY, connection_state::RECONNECTING);
      add_transition(connection_state::CONNECTING, connection_event::SEND_FAILURE_RETRY, connection_state::RECONNECTING);
      // for closing listeners/force closing connections/etc..
      add_transition(connection_state::CONNECTING, connection_event::DISCONNECT_SUCCESS, connection_state::DISCONNECTED);
      add_transition(connection_state::CONNECTING, connection_event::DISCONNECT_FAILURE, connection_state::DISCONNECTED);

      add_transition(connection_state::RECONNECTING, connection_event::CONNECT_SUCCESS, connection_state::CONNECTED);
      add_transition(connection_state::RECONNECTING, connection_event::CONNECT_FAILURE_NO_RETRY, connection_state::DISCONNECTED);
      add_transition(connection_state::RECONNECTING, connection_event::CONNECT_FAILURE_RETRY, connection_state::RECONNECTING);

      add_transition(connection_state::CONNECTED, connection_event::CONNECTION_LOST_NO_RETRY, connection_state::RECONNECTING);
      add_transition(connection_state::CONNECTED, connection_event::CONNECTION_LOST_RETRY, connection_state::RECONNECTING);
      add_transition(connection_state::CONNECTED, connection_event::START_DISCONNECT, connection_state::DISCONNECTING);
      add_transition(connection_state::CONNECTED, connection_event::DISCONNECT_SUCCESS, connection_state::DISCONNECTED);
      add_transition(connection_state::CONNECTED, connection_event::READ_SUCCESS, connection_state::CONNECTED);
      // add_transition(connection_state::CONNECTED, connection_event::READ_FAILURE, connection_state::RECONNECTING);
      // add_transition(connection_state::CONNECTED, connection_event::SEND_FAILURE_NO_RETRY, connection_state::RECONNECTING);
      // add_transition(connection_state::CONNECTED, connection_event::SEND_FAILURE_RETRY, connection_state::RECONNECTING);

      add_transition(connection_state::DISCONNECTING, connection_event::DISCONNECT_SUCCESS, connection_state::DISCONNECTED);
      add_transition(connection_state::DISCONNECTING, connection_event::DISCONNECT_FAILURE, connection_state::DISCONNECTED);
    }
  };

}  // namespace other

#endif  // OTHERLIB_NETWORK_TCP_CONNECTION_STATE_MACHING_HPP
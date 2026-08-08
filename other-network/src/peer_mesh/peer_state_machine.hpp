/**
 * \file peer_mesh/peer_state_machine.hpp
 **/
#ifndef OTHER_NETWORK_PEER_MESH_PEER_STATE_MACHINE_HPP
#define OTHER_NETWORK_PEER_MESH_PEER_STATE_MACHINE_HPP

#include <cstdint>

#include "core/profiler.hpp"
#include "core/state_machine.hpp"

namespace other {

  enum class role_state : uint8_t {
    UNJOINED,
    JOINING,
    PEER,
    SERVER_CANDIDATE,
    SERVER,
    MIGRATING_OUT,
    LEAVING,

    NUM_STATES
  };

  enum class role_event : uint8_t {
    HANDSHAKE_STARTED,
    HANDSHAKE_COMPLETED,
    NOMINATE,
    PROMOTE,
    DEMOTE,
    DRAIN_COMPLETED,
    LEAVE_REQUESTED,
    DISCONNECT,

    NUM_EVENTS
  };

  class peer_state_machine : public state_machine<role_state, role_event> {
   public:
    peer_state_machine()
        : state_machine(role_state::UNJOINED) {
      PROFILE_SECTION("peer_state_machine::peer_state_machine");
      add_transition(role_state::UNJOINED, role_event::HANDSHAKE_STARTED, role_state::JOINING);
      add_transition(role_state::JOINING, role_event::HANDSHAKE_COMPLETED, role_state::PEER);
      /// the degenerate no-election path: a hosting session promotes itself directly
      add_transition(role_state::UNJOINED, role_event::PROMOTE, role_state::SERVER);
      add_transition(role_state::PEER, role_event::NOMINATE, role_state::SERVER_CANDIDATE);
      add_transition(role_state::SERVER_CANDIDATE, role_event::PROMOTE, role_state::SERVER);
      add_transition(role_state::SERVER_CANDIDATE, role_event::DEMOTE, role_state::PEER);
      add_transition(role_state::SERVER, role_event::LEAVE_REQUESTED, role_state::MIGRATING_OUT);
      add_transition(role_state::MIGRATING_OUT, role_event::DRAIN_COMPLETED, role_state::LEAVING);

      for (auto state : { role_state::UNJOINED, role_state::JOINING, role_state::PEER,
                          role_state::SERVER_CANDIDATE, role_state::SERVER, role_state::MIGRATING_OUT }) {
        add_transition(state, role_event::DISCONNECT, role_state::UNJOINED);
      }
    }

   private:
    void on_enter_state(role_state state) override;
  };

}  // namespace other

#endif  // OTHER_NETWORK_PEER_MESH_PEER_STATE_MACHINE_HPP
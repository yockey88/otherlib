/**
 * \file peer_mesh/peer_state_machine.cpp
 **/
#include "peer_mesh/peer_state_machine.hpp"

#include "core/profiler.hpp"

namespace other {

  void peer_state_machine::on_enter_state(role_state state) {
    PROFILE_SECTION("peer_state_machine::on_enter_state");
    switch (state) {
      case role_state::UNJOINED:
        break;
      case role_state::JOINING:
        break;
      case role_state::PEER:
        break;
      case role_state::SERVER_CANDIDATE:
        break;
      case role_state::SERVER:
        break;
      case role_state::MIGRATING_OUT:
        break;
      case role_state::LEAVING:
        break;
      default:
        OTHER_ASSERT(false, "Unhandled state in on_enter_state: {}", static_cast<uint8_t>(state));
    }
  }

}  // namespace other
/**
 * \file peer_mesh/peer_state_machine.cpp
 **/
#include "peer_mesh/peer_state_machine.hpp"

#include "core/logger.hpp"

namespace other {

  void peer_state_machine::on_enter_state(role_state state) {
    /// role transitions are load-bearing session moments; engine events ride the
    ///  session observer, this is the module-level trace
    CORE_LOG_DEBUG("peer role -> {}", static_cast<uint8_t>(state));
  }

}  // namespace other
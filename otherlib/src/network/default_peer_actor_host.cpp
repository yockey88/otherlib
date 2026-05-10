/**
 * \file network/default_peer_actor_host.cpp
 **/
#include "network/default_peer_actor_host.hpp"

namespace other {

  void default_peer_actor_host::tx_data(natural_t peer_id, std::span<const uint8_t> data) {
  }

  peer_actor::peer_metadata default_peer_actor_host::get_peer_metadata(natural_t peer_id) const {
    return {};
  }

  void default_peer_actor_host::request_disconnect(natural_t peer_id, std::error_code reason) {
  }

}  // namespace other
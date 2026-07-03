/**
 * \file peer-mesh/peer_record.hpp
 **/
#ifndef OTHER_NETWORK_PEER_MESH_PEER_RECORD_HPP
#define OTHER_NETWORK_PEER_MESH_PEER_RECORD_HPP

#include "core/defines.hpp"
#include "core/time.hpp"

namespace other {

  struct peer_record {
    natural_t peer_id = 0;
    natural_t connection_id = 0;

    // 32-bits available for client applications to specify role
    uint32_t role_mask = 0;

    std::string transport_name;
    microseconds rtt_estimate{ 0 };

    natural_t last_seen_tick = 0;
    std::vector<uint8_t> peer_metadata;
  };

}  // namespace other

#endif  // OTHER_NETWORK_PEER_MESH_PEER_RECORD_HPP
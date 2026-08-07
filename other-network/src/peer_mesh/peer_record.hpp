/**
 * \file peer_mesh/peer_record.hpp
 **/
#ifndef OTHER_NETWORK_PEER_MESH_PEER_RECORD_HPP
#define OTHER_NETWORK_PEER_MESH_PEER_RECORD_HPP

#include "core/defines.hpp"
#include "core/time.hpp"

#include "peer_mesh/node_id.hpp"

namespace other {

  /// a node the mesh knows about — resident (a local actor animates it) or remote
  ///  (reached over links). session layers fill the alias; the mesh never reads it
  struct peer_record {
    node_id node = 0;
    bool resident = false;

    /// 32-bits available for client applications to specify role
    uint32_t role_mask = 0;

    /// session-layer compact alias (0 = unassigned/host by session convention)
    uint16_t session_alias = 0;

    std::string display_name;
    microseconds rtt_estimate{ 0 };

    natural_t last_seen_tick = 0;
    ostd::vector<uint8_t> peer_metadata;
  };

}  // namespace other

#endif  // OTHER_NETWORK_PEER_MESH_PEER_RECORD_HPP

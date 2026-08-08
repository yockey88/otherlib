/**
 * \file peer_mesh/mesh_filter.hpp
 **/
#ifndef OTHER_NETWORK_PEER_MESH_MESH_FILTER_HPP
#define OTHER_NETWORK_PEER_MESH_MESH_FILTER_HPP

#include <functional>
#include <span>

#include "core/defines.hpp"

#include "peer_mesh/link.hpp"
#include "peer_mesh/node_id.hpp"

namespace other {

  struct mesh_frame_view {
    const link_record& link;
    node_id src = 0;
    node_id dst = 0;
    uint16_t net_id = 0;

    /// payload bytes are opaque; a filter may rewrite by filling scratch and
    ///  repointing payload at it (scratch outlives the chain for this frame)
    std::span<const uint8_t> payload;
    ostd::vector<uint8_t> scratch;

    /// stops the chain and the delivery/forward
    bool dropped = false;
  };

  using mesh_filter = std::function<void(mesh_frame_view&)>;

}  // namespace other

#endif  // OTHER_NETWORK_PEER_MESH_MESH_FILTER_HPP

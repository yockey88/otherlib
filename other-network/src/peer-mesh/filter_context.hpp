/**
 * \file filter_context.hpp
 **/
#ifndef OTHER_NETWORK_PEER_MESH_FILTER_CONTEXT_HPP
#define OTHER_NETWORK_PEER_MESH_FILTER_CONTEXT_HPP

#include <span>

namespace other {

  class packet_sink;

  struct filter_context {
    const natural_t peer_id;
    const natural_t conn_id;

    bool decoded = false;

    // message_view msg;
    std::span<const uint8_t> bytes;

    // force subsequent filters to skip packet
    bool dropped = false;

    struct emit_target {
      packet_sink* sink;
    };
    ostd::vector<emit_target> emit_targets;  // reused; not allocated per packet
  };

}  // namespace other

#endif  // OTHER_NETWORK_PEER_MESH_FILTER_CONTEXT_HPP
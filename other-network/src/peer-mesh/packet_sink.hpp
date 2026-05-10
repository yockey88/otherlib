/**
 * \file peer-mesh/packet_sink.hpp
 **/
#ifndef OTHER_NETWORK_PEER_MESH_PACKET_SINK_HPP
#define OTHER_NETWORK_PEER_MESH_PACKET_SINK_HPP

#include "core/defines.hpp"

namespace other {

  class packet_sink {
   public:
    virtual ~packet_sink() = default;

    virtual void rx_data(natural_t from_peer_id, std::vector<uint8_t> data) = 0;
    virtual void on_connection_opened(natural_t peer_id) = 0;
    virtual void on_connection_closed(natural_t peer_id) = 0;
  };

}  // namespace other

#endif  // OTHER_NETWORK_PEER_MESH_PACKET_SINK_HPP
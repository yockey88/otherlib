/**
 * \file peer_mesh/link_transport.hpp
 **/
#ifndef OTHER_NETWORK_PEER_MESH_LINK_TRANSPORT_HPP
#define OTHER_NETWORK_PEER_MESH_LINK_TRANSPORT_HPP

#include <functional>
#include <span>

#include "core/defines.hpp"

#include "network/net_address.hpp"
#include "peer_mesh/link.hpp"

namespace other {

  /// the byte-mover surface a mesh consumes — bytes in, bytes out, no frames, no interpretation.
  ///  connection ids are allocated by the implementation and unique within it
  class link_transport {
   public:
    struct callbacks {
      std::function<void(natural_t conn_id)> opened;
      std::function<void(natural_t listener_id, natural_t conn_id)> accepted;
      std::function<void(natural_t conn_id, std::span<const uint8_t> bytes)> received;
      std::function<void(natural_t conn_id)> closed;
    };

    virtual ~link_transport() = default;

    virtual std::string_view name() const = 0;
    /// stream transports deliver chunks the network reassembles; message/datagram
    ///  transports deliver whole blobs mapped 1 delivery = 1 frame
    virtual bool is_stream() const = 0;
    virtual link_caps conn_caps(natural_t conn_id) const = 0;

    /// platform-authenticated remote node id for a connection; 0 = the transport
    ///  attests nothing (tcp/udp/memory). a nonzero value must match LINK_HELLO
    virtual node_id attested_remote(natural_t conn_id) const { return 0; }

    /// one consumer per instance; events fire from the implementation's tick/pump
    virtual void bind(callbacks cbs) = 0;

    /// 0 = refused; establishment completion arrives via callbacks
    virtual natural_t dial(const net_address& remote) = 0;
    virtual natural_t listen(const net_address& bind_addr) = 0;

    virtual void tx(natural_t conn_id, std::span<const uint8_t> bytes) = 0;
    virtual void close(natural_t conn_id) = 0;
  };

}  // namespace other

#endif  // OTHER_NETWORK_PEER_MESH_LINK_TRANSPORT_HPP

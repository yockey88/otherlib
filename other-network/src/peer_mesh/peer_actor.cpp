/**
 * \file peer_mesh/peer_actor.cpp
 **/
#include "peer_mesh/peer_actor.hpp"

#include "peer_mesh/peer_mesh.hpp"

namespace other {

  peer_mesh& peer_actor::mesh() const {
    OTHER_ASSERT(owner != nullptr, "Actor '{}' is not spawned on a mesh.", name());
    return *owner;
  }

  natural_t peer_actor::open_link(const net_address& remote, std::string_view transport) {
    return mesh().open_link_from(*this, remote, transport);
  }

  natural_t peer_actor::open_listener(const net_address& bind, std::string_view transport) {
    return mesh().open_listener_from(*this, bind, transport);
  }

  void peer_actor::close_link(natural_t link_id, link_close_reason reason) {
    mesh().close_link(link_id, reason);
  }

  ostd::vector<link_record> peer_actor::links() const {
    return mesh().links_of(node);
  }

  bool peer_actor::send(node_id dst, uint16_t net_id, std::span<const uint8_t> payload) {
    return mesh().send_from(*this, dst, net_id, payload);
  }

  bool peer_actor::send_on_link(natural_t link_id, uint16_t net_id, std::span<const uint8_t> payload) {
    return mesh().send_on_link_from(*this, link_id, net_id, payload);
  }

}  // namespace other

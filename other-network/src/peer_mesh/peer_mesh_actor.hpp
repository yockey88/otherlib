/**
 * \file peer_mesh/peer_mesh_actor.hpp
 **/
#ifndef OTHER_NETWORK_PEER_MESH_PEER_MESH_ACTOR_HPP
#define OTHER_NETWORK_PEER_MESH_PEER_MESH_ACTOR_HPP

#include <span>

#include "core/defines.hpp"
#include "core/interfaces.hpp"
#include "core/time.hpp"

#include "network/net_address.hpp"
#include "peer_mesh/link.hpp"
#include "peer_mesh/node_id.hpp"

namespace other {

  class peer_mesh;

  /// a node in the graph. local actors are spawned and ticked by a mesh; remote nodes
  ///  are graph records reached over links. payload bytes are opaque both directions —
  ///  interpreting them is the actor's job, on_frame is the whole mailbox contract
  class OTHER_CLASS peer_mesh_actor {
    OTHER_ENVIRONMENT_INTERFACE("Network", "PeerMeshActor");

   public:
    virtual ~peer_mesh_actor() = default;

    virtual std::string_view name() const = 0;

    node_id id() const { return node; }
    bool spawned() const { return owner != nullptr; }
    peer_mesh& mesh() const;

    /// topology operations from this actor's seat — the mesh executes, the actor decides.
    ///  transport "" resolves by address kind
    natural_t open_link(const net_address& remote, std::string_view transport = "");
    natural_t open_listener(const net_address& bind, std::string_view transport = "");
    void close_link(natural_t link_id, link_close_reason reason);
    ostd::vector<link_record> links() const;

    /// dst link-adjacent from this seat -> direct frame; else routed via the mesh
    ///  router, or false. control-page net_ids are refused
    bool send(node_id dst, uint16_t net_id, std::span<const uint8_t> payload);
    /// raw single-hop on one of this actor's links
    bool send_on_link(natural_t link_id, uint16_t net_id, std::span<const uint8_t> payload);

    virtual void on_frame(const link_record& via, node_id src, uint16_t net_id, std::span<const uint8_t> payload) = 0;
    virtual void on_link_up(const link_record& link) {}
    virtual void on_link_down(const link_record& link, link_close_reason reason) {}
    virtual void tick(microseconds now, double dt) {}

   private:
    friend class peer_mesh;

    peer_mesh* owner = nullptr;
    node_id node = 0;
  };

}  // namespace other

#endif  // OTHER_NETWORK_PEER_MESH_PEER_MESH_ACTOR_HPP

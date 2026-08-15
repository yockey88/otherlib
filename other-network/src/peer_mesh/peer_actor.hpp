/**
 * \file peer_mesh/peer_actor.hpp
 **/
#ifndef OTHER_NETWORK_PEER_MESH_PEER_ACTOR_HPP
#define OTHER_NETWORK_PEER_MESH_PEER_ACTOR_HPP

#include <span>

#include "core/defines.hpp"
#include "core/interfaces.hpp"
#include "core/time.hpp"

#include "network/net_address.hpp"
#include "network/link.hpp"
#include "network/node_id.hpp"

namespace other {

  class peer_mesh;

  /// a node in a network. the primary actor is the local endpoint; a secondary is the
  ///  mesh's record — and optionally behavior — for a remote endpoint, which may be
  ///  simulated locally. the base is concrete: an inert record secondary is just this.
  ///  payloads are opaque — on_frame is the whole mailbox contract
  class OTHER_CLASS peer_actor {
    OTHER_ENVIRONMENT_INTERFACE("Network", "PeerActor");

   public:
    virtual ~peer_actor() = default;

    virtual std::string_view name() const { return "peer"; }

    node_id id() const { return node; }
    bool spawned() const { return owner != nullptr; }
    bool is_primary() const { return primary; }
    peer_mesh& mesh() const;

    /// topology operations from this actor's seat — the mesh executes, the actor decides.
    ///  transport "" resolves by address kind
    natural_t open_link(const net_address& remote, std::string_view transport = "");
    natural_t open_listener(const net_address& bind, std::string_view transport = "");
    void close_link(natural_t link_id, link_close_reason reason);
    ostd::vector<link_record> links() const;

    /// the UP link between this seat and dst, else refused (counted). control-page
    ///  net_ids are refused; multi-hop is actor behavior, not a mesh service
    bool send(node_id dst, uint16_t net_id, std::span<const uint8_t> payload);
    /// raw single-hop on one of this actor's links
    bool send_on_link(natural_t link_id, uint16_t net_id, std::span<const uint8_t> payload);

    virtual void on_frame(const link_record& via, node_id src, uint16_t net_id, std::span<const uint8_t> payload) {}
    virtual void on_link_up(const link_record& link) {}
    virtual void on_link_down(const link_record& link, link_close_reason reason) {}
    virtual void tick(microseconds now, double dt) {}

   private:
    friend class peer_mesh;

    peer_mesh* owner = nullptr;
    node_id node = 0;
    bool primary = false;
    /// minted by the mesh at link-up for an unknown remote; reaped with its last link
    bool auto_spawned = false;
  };

}  // namespace other

#endif  // OTHER_NETWORK_PEER_MESH_PEER_ACTOR_HPP

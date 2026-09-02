/**
 * \file peer_mesh/peer_actor.hpp
 **/
#ifndef OTHER_NETWORK_PEER_MESH_PEER_ACTOR_HPP
#define OTHER_NETWORK_PEER_MESH_PEER_ACTOR_HPP

#include <span>

#include "core/defines.hpp"
#include "core/interfaces.hpp"
#include "core/time.hpp"

#include "network/link.hpp"
#include "network/net_address.hpp"
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

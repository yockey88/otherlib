/**
 * \file peer_mesh/peer_mesh.hpp
 **/
#ifndef OTHER_NETWORK_PEER_MESH_PEER_MESH_HPP
#define OTHER_NETWORK_PEER_MESH_PEER_MESH_HPP

#include <functional>
#include <string>

#include "core/defines.hpp"
#include "core/scope.hpp"
#include "core/time.hpp"

#include "network/link.hpp"
#include "network/link_security.hpp"
#include "network/net_address.hpp"
#include "network/node_id.hpp"

#include "peer_mesh/peer_actor.hpp"

namespace other {

  struct peer_mesh_config {
    natural_t app_hash = 0;
    natural_t max_links = 32;
    microseconds handshake_timeout{ 3'000'000 };
    microseconds keepalive_idle{ 5'000'000 };
    microseconds link_timeout{ 15'000'000 };
  };

  class peer_mesh {
   public:
   private:
  };

}  // namespace other

#endif  // OTHER_NETWORK_PEER_MESH_PEER_MESH_HPP

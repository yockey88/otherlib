/**
 * \file network/default_peer_actor_host.hpp
 **/
#ifndef OTHERLIB_NETWORK_DEFAULT_PEER_ACTOR_HOST_HPP
#define OTHERLIB_NETWORK_DEFAULT_PEER_ACTOR_HOST_HPP

#include "peer-mesh/peer_actor_host.hpp"

namespace other {

  class default_peer_actor_host : public peer_actor_host {
   public:
    default_peer_actor_host(job_system& jobs)
        : peer_actor_host(jobs) {}
    ~default_peer_actor_host() = default;

    void tx_data(natural_t peer_id, std::span<const uint8_t> data) override;

    peer_actor::peer_metadata get_peer_metadata(natural_t peer_id) const override;
    void request_disconnect(natural_t peer_id, std::error_code reason) override;
  };

}  // namespace other

#endif  // OTHERLIB_NETWORK_DEFAULT_PEER_ACTOR_HOST_HPP
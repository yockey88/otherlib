/**
 * \file peer-mesh/peer_actor_host.hpp
 **/
#ifndef OTHER_NETWORK_PEER_MESH_PEER_ACTOR_HOST_HPP
#define OTHER_NETWORK_PEER_MESH_PEER_ACTOR_HOST_HPP

#include <vector>

#include "core/defines.hpp"
#include "core/job_system.hpp"
#include "core/ref_counted.hpp"

#include "message/message.hpp"

#include "peer-mesh/peer_actor.hpp"


namespace other {

  class peer_actor_host : public ref_counted {
   public:
    peer_actor_host(job_system& jobs)  //, const binding_point& bp)
        : jobs(jobs) {}
    virtual ~peer_actor_host() = default;

    void post_to_main(std::function<void()> func);

    virtual void tx_data(natural_t peer_id, std::span<const uint8_t> data) = 0;
    virtual peer_actor::peer_metadata get_peer_metadata(natural_t peer_id) const = 0;
    virtual void request_disconnect(natural_t peer_id, std::error_code reason) = 0;

   private:
    job_system& jobs;
  };

}  // namespace other

#endif  // OTHER_NETWORK_PEER_MESH_PEER_ACTOR_HOST_HPP
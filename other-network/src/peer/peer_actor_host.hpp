/**
 * \file peer/peer_actor_host.hpp
 **/
#ifndef OTHER_NETWORK_PEER_PEER_ACTOR_HOST_HPP
#define OTHER_NETWORK_PEER_PEER_ACTOR_HOST_HPP

#include <vector>

#include "core/defines.hpp"

namespace other {

  class peer_actor_host {
   public:
    virtual ~peer_actor_host() = default;

    virtual void tx_data(natural_t to_peer_id, std::vector<uint8_t> data) = 0;
    virtual void post_to_main_thread(std::function<void()> work_fn) = 0;
  };

}  // namespace other

#endif  // OTHER_NETWORK_PEER_PEER_ACTOR_HOST_HPP
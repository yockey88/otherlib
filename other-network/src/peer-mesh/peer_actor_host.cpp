/**
 * \file peer-mesh/peer_actor_host.cpp
 **/
#include "peer-mesh/peer_actor_host.hpp"

namespace other {

  void peer_actor_host::post_to_main(std::function<void()> func) {
    jobs.submit(
      {
        .name = "Peer Actor Host Main Thread Task",
        .priority = job::priority::HIGH,
        .thread_affinity = job::affinity::MAIN_THREAD,
      },
      [func = std::move(func)]() {
        func();
      }
    );
  }

}  // namespace other
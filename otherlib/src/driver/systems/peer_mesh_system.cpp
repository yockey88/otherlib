/**
 * \file driver/systems/peer_mesh_system.cpp
 **/
#include "driver/systems/peer_mesh_system.hpp"

#include "network/default_peer_actor_host.hpp"

#include "driver/systems/job_driver_system.hpp"

namespace other {

  void peer_mesh_system::register_peer_actor_host(ref<peer_actor_host> host) {
    actor_host = host;
  }

  void peer_mesh_system::initialize(driver_kernel* kernel) {
    if (actor_host == nullptr) {
      job_system& jobs = sibling<job_driver_system>(*kernel).get_job_system();
      register_peer_actor_host(make_ref<default_peer_actor_host>(jobs));
    }
    OTHER_ASSERT(actor_host != nullptr, "Peer actor host must be set during peer mesh system initialization.");
  }

  void peer_mesh_system::tick(driver_kernel* kernel, double dt) {
  }

  void peer_mesh_system::shutdown(driver_kernel* kernel) {
  }

}  // namespace other
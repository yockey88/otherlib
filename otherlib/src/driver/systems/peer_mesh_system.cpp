/**
 * \file driver/systems/peer_mesh_system.cpp
 **/
#include "driver/systems/peer_mesh_system.hpp"

#include "driver/systems/job_driver_system.hpp"

namespace other {

  void peer_mesh_system::initialize(driver_kernel* kernel) {
    OTHER_ASSERT(kernel != nullptr, "Kernel is null in peer_mesh_system::initialize.");
    PROFILE_SECTION("peer_mesh_system::initialize");
  }

  void peer_mesh_system::tick(driver_kernel* kernel, double dt) {
    PROFILE_SECTION("peer_mesh_system::tick");
  }

  void peer_mesh_system::shutdown(driver_kernel* kernel) {
    PROFILE_SECTION("peer_mesh_system::shutdown");
  }

}  // namespace other
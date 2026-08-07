/**
 * \file driver/systems/peer_mesh_system.hpp
 **/
#ifndef OTHERLIB_DRIVER_SYSTEMS_PEER_MESH_SYSTEM_HPP
#define OTHERLIB_DRIVER_SYSTEMS_PEER_MESH_SYSTEM_HPP

#include "core/ref.hpp"
#include "core/scope.hpp"

#include "driver/systems/core_system.hpp"

#include "peer_mesh/peer_graph.hpp"

namespace other {

  class OTHER_CLASS peer_mesh_system : public core_system<peer_mesh_system> {
   public:
    peer_mesh_system(driver* driver)
        : core_system(driver, driver_system_type::PEER_MESH_DRIVER_SYSTEM) {}
    ~peer_mesh_system() override {}

    std::string name() const override { return "Peer Mesh System"; }

    void initialize(driver_kernel* kernel) override;
    void tick(driver_kernel* kernel, double dt) override;
    void shutdown(driver_kernel* kernel) override;

   private:
  };

}  // namespace other

#endif  // OTHERLIB_DRIVER_SYSTEMS_PEER_MESH_SYSTEM_HPP
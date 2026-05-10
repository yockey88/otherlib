/**
 * \file driver/systems/peer_mesh_system.hpp
 **/
#ifndef OTHERLIB_DRIVER_SYSTEMS_PEER_MESH_SYSTEM_HPP
#define OTHERLIB_DRIVER_SYSTEMS_PEER_MESH_SYSTEM_HPP

#include "core/ref.hpp"
#include "core/scope.hpp"

#include "driver/systems/core_system.hpp"

#include "peer-mesh/peer_actor_host.hpp"
#include "peer-mesh/peer_graph.hpp"
#include "peer-mesh/peer_mailbox.hpp"

namespace other {

  class peer_mesh_system : public core_system<peer_mesh_system> {
   public:
    peer_mesh_system(driver* driver)
        : core_system(driver, driver_system_type::PEER_MESH_DRIVER_SYSTEM) {}
    ~peer_mesh_system() override {}

    std::string name() const override { return "Peer Mesh System"; }

    void register_peer_actor_host(ref<peer_actor_host> host);

    void initialize(driver_kernel* kernel) override;
    void tick(driver_kernel* kernel, double dt) override;
    void shutdown(driver_kernel* kernel) override;

   private:
    struct actor_state {
      scope<peer_actor> actor;
      peer_mailbox mailbox;
      std::atomic<bool> scheduled{ false };
      std::atomic<bool> alive{ true };
    };

    ref<peer_actor_host> actor_host;

    peer_graph graph;

    std::unordered_map<natural_t, actor_state> peer_actors;
    std::unordered_map<natural_t, natural_t> connection_id_to_peer_id;
  };

}  // namespace other

#endif  // OTHERLIB_DRIVER_SYSTEMS_PEER_MESH_SYSTEM_HPP
/**
 * \file driver/systems/peer_mesh_system.hpp
 **/
#ifndef OTHERLIB_DRIVER_SYSTEMS_PEER_MESH_SYSTEM_HPP
#define OTHERLIB_DRIVER_SYSTEMS_PEER_MESH_SYSTEM_HPP

#include "core/scope.hpp"
#include "core/value.hpp"

#include "driver/systems/core_system.hpp"

#include "network/session/network_session.hpp"
#include "peer_mesh/peer_mesh.hpp"
#include "peer_mesh/provider_link_transport.hpp"

namespace other {

  /// owns the driver's default mesh and spawns the default session actor on it —
  ///  resolved by `networking.session-host` so plugin DLLs ship custom actors.
  ///  inert when networking is force-disabled
  class OTHER_CLASS peer_mesh_system : public core_system<peer_mesh_system> {
   public:
    peer_mesh_system(driver* driver)
        : core_system(driver, driver_system_type::PEER_MESH_DRIVER_SYSTEM) {}
    ~peer_mesh_system() override {}

    std::string name() const override { return "Peer Mesh System"; }

    void initialize(driver_kernel* kernel) override;
    void late_initialize(driver_kernel* kernel) override;
    void tick(driver_kernel* kernel, double dt) override;
    void shutdown(driver_kernel* kernel) override;

    /// the default session actor, when it is the built-in client-server one
    network_session* session() { return active_session; }
    peer_mesh* mesh() { return driver_mesh.get(); }

    bool host_session(uint16_t port);
    bool join_session(const std::string_view address_text, uint16_t port);

    /// GAME_EVENT payload handoff for the C# pull (primitives-only invoke marshal)
    size_t pending_event_payload_size() const { return pending_event_payload.size(); }
    size_t copy_pending_event_payload(uint8_t* dst, size_t capacity);

   private:
    scope<peer_mesh> driver_mesh;
    scope<provider_link_transport> tcp_link;
    network_session* active_session = nullptr;
    node_id session_node = 0;
    /// provider id of a spawned registry-provided actor; plugin revoke destroys it
    natural_t spawned_provider = 0;
    session_actor_source actors;

    microseconds engine_now{ 0 };
    bool networking_off = false;
    ostd::vector<uint8_t> pending_event_payload;

    void build(driver_kernel* kernel);
    void handle_network_command(const value& data);
    void on_session_event(session_event ev, uint16_t arg);
    void dispatch_game_event(uint16_t sender_peer, std::string_view event_name, std::span<const uint8_t> payload);
    std::string status_text() const;
  };

}  // namespace other

#endif  // OTHERLIB_DRIVER_SYSTEMS_PEER_MESH_SYSTEM_HPP

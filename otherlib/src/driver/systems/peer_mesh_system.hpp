/**
 * \file driver/systems/peer_mesh_system.hpp
 **/
#ifndef OTHERLIB_DRIVER_SYSTEMS_PEER_MESH_SYSTEM_HPP
#define OTHERLIB_DRIVER_SYSTEMS_PEER_MESH_SYSTEM_HPP

#include "core/scope.hpp"
#include "core/value.hpp"

#include "driver/systems/core_system.hpp"

#include "network/session/network_session.hpp"
#include "network/session/replication.hpp"
#include "network/session/scene_ops.hpp"
#include "peer_mesh/peer_mesh.hpp"
#include "peer_mesh/provider_link_transport.hpp"

#include "steam/steam_link_transport.hpp"

namespace other {

  class steam_context;

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
    replication* replicator() { return scene_replication.get(); }
    op_channel* ops() { return scene_ops.get(); }

    /// honors networking.transport: "steam" hosts a lobby + P2P listen, else tcp
    bool host_session(uint16_t port);
    bool join_session(const std::string_view address_text, uint16_t port);
    bool host_steam_session();
    bool join_lobby(uint64_t lobby_id);
    void open_invite_dialog();

    /// GAME_EVENT payload handoff for the C# pull (primitives-only invoke marshal)
    size_t pending_event_payload_size() const { return pending_event_payload.size(); }
    size_t copy_pending_event_payload(uint8_t* dst, size_t capacity);
    /// the reverse direction: C# stages its [Replicated] blob during a collect call
    void stage_script_fields(const uint8_t* data, size_t length) {
      staged_script_fields.assign(data, data + length);
    }

   private:
    scope<peer_mesh> driver_mesh;
    scope<provider_link_transport> tcp_link;
    scope<steam_link_transport> steam_link;
    /// borrowed from network_system, set only when READY; it outlives this system
    steam_context* steam_ctx = nullptr;
    network_session* active_session = nullptr;
    scope<replication> scene_replication;
    scope<op_channel> scene_ops;
    node_id session_node = 0;
    /// provider id of a spawned registry-provided actor; plugin revoke destroys it
    natural_t spawned_provider = 0;
    session_actor_source actors;

    microseconds engine_now{ 0 };
    bool networking_off = false;
    ostd::vector<uint8_t> pending_event_payload;
    ostd::vector<uint8_t> staged_script_fields;

    /// Mode 1: authored playback state (05 §6 lifecycle — stop leaves the session)
    bool scene_was_playing = false;
    bool authored_active = false;
    std::string authored_spawn_template;

    void build(driver_kernel* kernel);
    void handle_network_command(const value& data);
    void handle_lobby_join_request(uint64_t lobby_id);
    void watch_authored_playback(driver_kernel* kernel);
    void apply_authored_settings(scene& s);
    void spawn_template_for_peer(uint16_t peer);
    void on_session_event(session_event ev, uint16_t arg);
    void dispatch_game_event(uint16_t sender_peer, std::string_view event_name, std::span<const uint8_t> payload);
    op_result validate_op_via_scripts(uint16_t peer, const scene_op& op);
    void dispatch_op_to_scripts(std::string_view method, uint16_t actor, const std::string& op_name,
                                natural_t subject, std::span<const uint8_t> payload);
    std::string status_text();
    std::string journal_text();
  };

}  // namespace other

#endif  // OTHERLIB_DRIVER_SYSTEMS_PEER_MESH_SYSTEM_HPP

/**
 * \file scripting/dotnet_bindings/network_bindings.cpp
 **/
#include "scripting/dotnet_bindings/network_bindings.hpp"

#include <span>

#include "driver/driver.hpp"
#include "driver/systems/peer_mesh_system.hpp"
#include "scripting/dotnet_bindings/driver_bindings.hpp"

namespace other {
  namespace bindings {

    namespace {

      peer_mesh_system* mesh_system() {
        driver* d = detail::get_dotnet_native_driver_unchecked();
        if (d == nullptr) {
          return nullptr;
        }
        driver_kernel& kernel = d->get_kernel();
        return kernel.has_core_system<peer_mesh_system>() ? &kernel.get_core_system<peer_mesh_system>() : nullptr;
      }

      network_session* session() {
        // peer_mesh_system* system = mesh_system();
        // return system != nullptr ? system->session() : nullptr;
        return nullptr;
      }

      std::span<const uint8_t> payload_span(const uint8_t* payload, int32_t length) {
        return payload != nullptr && length > 0 ? std::span<const uint8_t>(payload, static_cast<size_t>(length)) : std::span<const uint8_t>{};
      }

    }  // namespace

    nbool32 native_network_is_connected() {
      network_session* s = session();
      return s != nullptr && s->in_session();
    }

    int32_t native_network_get_role() {
      network_session* s = session();
      if (s == nullptr || !s->in_session()) {
        return 0;  // None
      }
      return s->is_host() ? 1 : 2;  // Host : Client
    }

    int32_t native_network_local_peer_id() {
      network_session* s = session();
      return s != nullptr ? s->local_peer_id() : 0;
    }

    nbool32 native_network_host(uint16_t port) {
      // peer_mesh_system* system = mesh_system();
      // return system != nullptr && system->host_session(port);
      return false;
    }

    nbool32 native_network_join(native_string address, uint16_t port) {
      // peer_mesh_system* system = mesh_system();
      // return system != nullptr && system->join_session(static_cast<std::string>(address), port);
      return false;
    }

    void native_network_leave() {
      if (network_session* s = session(); s != nullptr) {
        s->leave();
      }
    }

    void native_network_send_event(native_string event_name, const uint8_t* payload, int32_t length) {
      if (network_session* s = session(); s != nullptr) {
        s->send_game_event(static_cast<std::string>(event_name), payload_span(payload, length));
      }
    }

    void native_network_broadcast_event(native_string event_name, const uint8_t* payload, int32_t length) {
      if (network_session* s = session(); s != nullptr) {
        s->broadcast_game_event(static_cast<std::string>(event_name), payload_span(payload, length));
      }
    }

    int32_t native_network_copy_event_payload(uint8_t* dst, int32_t capacity) {
      // peer_mesh_system* system = mesh_system();
      // if (system == nullptr || dst == nullptr || capacity <= 0) {
      //   return 0;
      // }
      // return static_cast<int32_t>(system->copy_pending_event_payload(dst, static_cast<size_t>(capacity)));
      return false;
    }

    nbool32 native_network_spawn(uint64_t object_id, uint16_t owner_peer) {
      // peer_mesh_system* system = mesh_system();
      // return system != nullptr && system->replicator() != nullptr &&
      //   system->replicator()->spawn_object(object_id, owner_peer);
      return false;
    }

    nbool32 native_network_sync_component(uint64_t object_id, native_string component_key) {
      // peer_mesh_system* system = mesh_system();
      // return system != nullptr && system->replicator() != nullptr &&
      //   system->replicator()->sync_component(object_id, FNV(static_cast<std::string>(component_key)));
      return false;
    }

    nbool32 native_network_is_mine(uint64_t object_id) {
      // peer_mesh_system* system = mesh_system();
      // return system != nullptr && system->replicator() != nullptr && system->replicator()->is_mine(object_id);
      return false;
    }

    nbool32 native_network_request_op(native_string op_name, uint64_t subject_net_id, const uint8_t* payload, int32_t length) {
      // peer_mesh_system* system = mesh_system();
      // return system != nullptr && system->ops() != nullptr &&
      //   system->ops()->request(static_cast<std::string>(op_name), subject_net_id, payload_span(payload, length));
      return false;
    }

    void native_network_stage_replicated(const uint8_t* data, int32_t length) {
      // if (peer_mesh_system* system = mesh_system(); system != nullptr && data != nullptr && length > 0) {
      //   system->stage_script_fields(data, static_cast<size_t>(length));
      // }
    }

    namespace {

      const session_member* member_at(int32_t index) {
        network_session* s = session();
        if (s == nullptr || index < 0 || static_cast<size_t>(index) >= s->peers().size()) {
          return nullptr;
        }
        return &s->peers()[static_cast<size_t>(index)];
      }

    }  // namespace

    int32_t native_network_peer_count() {
      network_session* s = session();
      return s != nullptr ? static_cast<int32_t>(s->peers().size()) : 0;
    }

    uint32_t native_network_peer_id(int32_t index) {
      const session_member* member = member_at(index);
      return member != nullptr ? member->peer_id : 0;
    }

    uint64_t native_network_peer_node(int32_t index) {
      const session_member* member = member_at(index);
      return member != nullptr ? member->node : 0;
    }

    native_string native_network_peer_name(int32_t index) {
      const session_member* member = member_at(index);
      return native_string::new_str(member != nullptr ? std::string_view(member->name) : std::string_view{});
    }

    void native_network_actor_stage_payload(const uint8_t* data, int32_t length) {
      // if (peer_mesh_system* system = mesh_system(); system != nullptr) {
      //   system->stage_actor_payload(data, length > 0 ? static_cast<size_t>(length) : 0);
      // }
    }

    nbool32 native_network_actor_send(uint64_t dst, uint32_t net_id) {
      // peer_mesh_system* system = mesh_system();
      // return system != nullptr && system->script_send(dst, static_cast<uint16_t>(net_id));
      return false;
    }

    uint64_t native_network_actor_open_link(native_string address, uint16_t port, native_string transport) {
      // peer_mesh_system* system = mesh_system();
      // return system != nullptr ? system->script_open_link(static_cast<std::string>(address), port, static_cast<std::string>(transport)) : 0;
      return 0;
    }

    uint64_t native_network_actor_open_listener(uint16_t port, native_string transport) {
      // peer_mesh_system* system = mesh_system();
      // return system != nullptr ? system->script_open_listener(port, static_cast<std::string>(transport)) : 0;
      return 0;
    }

    void native_network_actor_close_link(uint64_t link_id, uint32_t reason) {
      // if (peer_mesh_system* system = mesh_system(); system != nullptr) {
      //   system->script_close_link(link_id, static_cast<uint16_t>(reason));
      // }
    }

  }  // namespace bindings
}  // namespace other

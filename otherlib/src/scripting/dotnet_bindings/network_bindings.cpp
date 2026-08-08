/**
 * \file scripting/dotnet_bindings/network_bindings.cpp
 **/
#include "scripting/dotnet_bindings/network_bindings.hpp"

#include <span>

#include "scripting/dotnet_bindings/driver_bindings.hpp"

#include "driver/driver.hpp"
#include "driver/systems/peer_mesh_system.hpp"

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
        peer_mesh_system* system = mesh_system();
        return system != nullptr ? system->session() : nullptr;
      }

      std::span<const uint8_t> payload_span(const uint8_t* payload, int32_t length) {
        return payload != nullptr && length > 0 ? std::span<const uint8_t>(payload, static_cast<size_t>(length))
                                                : std::span<const uint8_t>{};
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
      peer_mesh_system* system = mesh_system();
      return system != nullptr && system->host_session(port);
    }

    nbool32 native_network_join(native_string address, uint16_t port) {
      peer_mesh_system* system = mesh_system();
      return system != nullptr && system->join_session(static_cast<std::string>(address), port);
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
      peer_mesh_system* system = mesh_system();
      if (system == nullptr || dst == nullptr || capacity <= 0) {
        return 0;
      }
      return static_cast<int32_t>(system->copy_pending_event_payload(dst, static_cast<size_t>(capacity)));
    }

  }  // namespace bindings
}  // namespace other

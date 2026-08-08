/**
 * \file scripting/dotnet_bindings/steam_bindings.cpp
 **/
#include "scripting/dotnet_bindings/steam_bindings.hpp"

#include "steam/steam_context.hpp"

#include "scripting/dotnet_bindings/driver_bindings.hpp"

#include "driver/driver.hpp"
#include "driver/systems/network_system.hpp"
#include "driver/systems/peer_mesh_system.hpp"

namespace other {
  namespace bindings {

    namespace {

      steam_context* steam() {
        driver* d = detail::get_dotnet_native_driver_unchecked();
        if (d == nullptr) {
          return nullptr;
        }
        driver_kernel& kernel = d->get_kernel();
        return kernel.has_core_system<network_system>() ? kernel.get_core_system<network_system>().steam() : nullptr;
      }

      peer_mesh_system* mesh_system() {
        driver* d = detail::get_dotnet_native_driver_unchecked();
        if (d == nullptr) {
          return nullptr;
        }
        driver_kernel& kernel = d->get_kernel();
        return kernel.has_core_system<peer_mesh_system>() ? &kernel.get_core_system<peer_mesh_system>() : nullptr;
      }

    }  // namespace

    nbool32 native_steam_is_available() {
      steam_context* ctx = steam();
      return ctx != nullptr && ctx->state() == steam_state::READY;
    }

    uint64_t native_steam_player_id() {
      steam_context* ctx = steam();
      return ctx != nullptr ? ctx->local_steam_id() : 0;
    }

    native_string native_steam_player_name() {
      steam_context* ctx = steam();
      return native_string::new_str(ctx != nullptr ? ctx->persona_name() : std::string_view{});
    }

    void native_steam_open_invite_dialog() {
      if (peer_mesh_system* system = mesh_system(); system != nullptr) {
        system->open_invite_dialog();
      }
    }

    nbool32 native_network_host_steam() {
      peer_mesh_system* system = mesh_system();
      return system != nullptr && system->host_steam_session();
    }

    nbool32 native_network_join_lobby(uint64_t lobby_id) {
      peer_mesh_system* system = mesh_system();
      return system != nullptr && system->join_lobby(lobby_id);
    }

  }  // namespace bindings
}  // namespace other

/**
 * \file steam/steam_context.hpp
 **/
#ifndef OTHER_STEAM_STEAM_STEAM_CONTEXT_HPP
#define OTHER_STEAM_STEAM_STEAM_CONTEXT_HPP

#include <string>

#include "core/defines.hpp"

#include "steam/steam_lobby.hpp"

namespace other {

  enum class steam_state : uint8_t {
    DISABLED,     // steam.enabled = false; nothing constructed
    UNAVAILABLE,  // enabled but init failed (no client, no appid): warned once, inert
    READY,
  };

  /// init/shutdown/callback pump/identity. degradation is the contract: no path
  ///  through here may abort boot, and CI (no steam client) never notices steam exists
  class steam_context {
   public:
    ~steam_context() { shutdown(); }

    /// SteamAPI_InitEx; the dev AppID (480) also writes steam_appid.txt beside the
    ///  process when missing — real AppIDs ship via project compilation settings (S6)
    steam_state initialize(uint32_t app_id);
    void shutdown();
    /// SteamAPI_RunCallbacks — main thread, every tick
    void pump();

    steam_state state() const { return current_state; }
    /// the mesh node id on steam links (attested); 0 unless READY
    uint64_t local_steam_id() const { return steam_id; }
    std::string_view persona_name() const { return persona; }
    steam_lobby& lobby() { return matchmaking; }

   private:
    steam_state current_state = steam_state::DISABLED;
    uint64_t steam_id = 0;
    std::string persona;
    steam_lobby matchmaking;
  };

}  // namespace other

#endif  // OTHER_STEAM_STEAM_STEAM_CONTEXT_HPP

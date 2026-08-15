/**
 * \file steam/steam_lobby.hpp
 **/
#ifndef OTHER_STEAM_STEAM_STEAM_LOBBY_HPP
#define OTHER_STEAM_STEAM_STEAM_LOBBY_HPP

#include <functional>

#include "core/defines.hpp"

#include <steam/steam_api.h>

namespace other {

  enum class lobby_state : uint8_t {
    IDLE,
    CREATING,
    HOSTING,
    JOINING,
    IN_LOBBY,
  };

  /// matchmaking + invite flow. steam replaces dial and discovery, never the protocol — the
  ///  hooks below are the only output. on_* entries take parsed values so tests can fake the callback driver
  class steam_lobby {
   public:
    struct hooks {
      /// a joined lobby resolved its owner — connect to this host
      std::function<void(uint64_t host_id)> ready_to_connect;
      /// overlay invite accepted / launch-by-invite — the glue decides auto-join
      std::function<void(uint64_t lobby_id)> join_requested;
    };

    void set_hooks(hooks h) { wired = std::move(h); }

    /// lobby_type = ELobbyType; both return false when already busy
    bool create(uint8_t lobby_type, int max_members);
    bool join(uint64_t lobby_id);
    void leave();
    void open_invite_dialog();

    lobby_state state() const { return current_state; }
    uint64_t lobby_id() const { return current_lobby; }
    /// live SDK query; 0 without a lobby or a READY steam context
    int member_count() const;

    /// callback entries (also the test seam)
    void on_lobby_created(bool ok, uint64_t lobby_id);
    void on_lobby_entered(uint64_t lobby_id, uint64_t owner_id, bool ok);
    void on_join_requested(uint64_t lobby_id);

   private:
    lobby_state current_state = lobby_state::IDLE;
    uint64_t current_lobby = 0;
    hooks wired;

    void on_create_result(LobbyCreated_t* result, bool io_failure);
    void on_enter_result(LobbyEnter_t* result, bool io_failure);
    CCallResult<steam_lobby, LobbyCreated_t> create_call;
    CCallResult<steam_lobby, LobbyEnter_t> enter_call;
    STEAM_CALLBACK(steam_lobby, on_overlay_join_requested, GameLobbyJoinRequested_t);
  };

}  // namespace other

#endif  // OTHER_STEAM_STEAM_STEAM_LOBBY_HPP

/**
 * \file steam/steam_lobby.cpp
 **/
#include "steam/steam_lobby.hpp"

#include "core/logger.hpp"

namespace other {

  bool steam_lobby::create(uint8_t lobby_type, int max_members) {
    if (current_state != lobby_state::IDLE) {
      CORE_LOG_WARN("[STEAM] lobby create refused: busy (state {})", static_cast<uint8_t>(current_state));
      return false;
    }
    current_state = lobby_state::CREATING;
    if (ISteamMatchmaking* matchmaking = SteamMatchmaking(); matchmaking != nullptr) {
      const SteamAPICall_t call = matchmaking->CreateLobby(static_cast<ELobbyType>(lobby_type), max_members);
      create_call.Set(call, this, &steam_lobby::on_create_result);
    }
    return true;
  }

  bool steam_lobby::join(uint64_t lobby_id) {
    if (current_state != lobby_state::IDLE) {
      CORE_LOG_WARN("[STEAM] lobby join refused: busy (state {})", static_cast<uint8_t>(current_state));
      return false;
    }
    current_state = lobby_state::JOINING;
    if (ISteamMatchmaking* matchmaking = SteamMatchmaking(); matchmaking != nullptr) {
      const SteamAPICall_t call = matchmaking->JoinLobby(CSteamID(lobby_id));
      enter_call.Set(call, this, &steam_lobby::on_enter_result);
    }
    return true;
  }

  void steam_lobby::leave() {
    if (current_lobby != 0) {
      if (ISteamMatchmaking* matchmaking = SteamMatchmaking(); matchmaking != nullptr) {
        matchmaking->LeaveLobby(CSteamID(current_lobby));
      }
    }
    current_lobby = 0;
    current_state = lobby_state::IDLE;
  }

  void steam_lobby::open_invite_dialog() {
    if (current_lobby == 0) {
      CORE_LOG_WARN("[STEAM] invite dialog refused: not in a lobby");
      return;
    }
    if (ISteamFriends* friends = SteamFriends(); friends != nullptr) {
      friends->ActivateGameOverlayInviteDialog(CSteamID(current_lobby));
    }
  }

  void steam_lobby::on_lobby_created(bool ok, uint64_t lobby_id) {
    if (current_state != lobby_state::CREATING) {
      return;
    }
    if (!ok) {
      CORE_LOG_WARN("[STEAM] lobby creation failed");
      current_state = lobby_state::IDLE;
      return;
    }
    current_lobby = lobby_id;
    current_state = lobby_state::HOSTING;
    CORE_LOG_INFO("[STEAM] hosting lobby {}", lobby_id);
  }

  void steam_lobby::on_lobby_entered(uint64_t lobby_id, uint64_t owner_id, bool ok) {
    if (current_state != lobby_state::JOINING) {
      return;
    }
    if (!ok || owner_id == 0) {
      CORE_LOG_WARN("[STEAM] lobby join failed");
      current_state = lobby_state::IDLE;
      return;
    }
    current_lobby = lobby_id;
    current_state = lobby_state::IN_LOBBY;
    CORE_LOG_INFO("[STEAM] entered lobby {} (host {:#x})", lobby_id, owner_id);
    if (wired.ready_to_connect != nullptr) {
      wired.ready_to_connect(owner_id);
    }
  }

  void steam_lobby::on_join_requested(uint64_t lobby_id) {
    if (wired.join_requested != nullptr) {
      wired.join_requested(lobby_id);
    }
  }

  /// ------------------------------------------------------------ live thunks

  void steam_lobby::on_create_result(LobbyCreated_t* result, bool io_failure) {
    on_lobby_created(!io_failure && result->m_eResult == k_EResultOK, result->m_ulSteamIDLobby);
  }

  void steam_lobby::on_enter_result(LobbyEnter_t* result, bool io_failure) {
    const bool ok = !io_failure && result->m_EChatRoomEnterResponse == k_EChatRoomEnterResponseSuccess;
    uint64_t owner_id = 0;
    if (ok) {
      if (ISteamMatchmaking* matchmaking = SteamMatchmaking(); matchmaking != nullptr) {
        owner_id = matchmaking->GetLobbyOwner(CSteamID(result->m_ulSteamIDLobby)).ConvertToUint64();
      }
    }
    on_lobby_entered(result->m_ulSteamIDLobby, owner_id, ok);
  }

  void steam_lobby::on_overlay_join_requested(GameLobbyJoinRequested_t* request) {
    on_join_requested(request->m_steamIDLobby.ConvertToUint64());
  }

  int steam_lobby::member_count() const {
    if (current_lobby == 0 || SteamMatchmaking() == nullptr) {
      return 0;
    }
    return SteamMatchmaking()->GetNumLobbyMembers(CSteamID(current_lobby));
  }

}  // namespace other

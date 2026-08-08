/**
 * \file steam/steam_context.cpp
 **/
#include "steam/steam_context.hpp"

#include <filesystem>
#include <fstream>

#include "core/logger.hpp"
#include "core/profiler.hpp"

#include <steam/steam_api.h>

namespace other {

  namespace {

    constexpr uint32_t kSpacewarDevAppId = 480;

  }  // namespace

  steam_state steam_context::initialize(uint32_t app_id) {
    PROFILE_SECTION("steam_context::initialize");
    if (current_state != steam_state::DISABLED) {
      return current_state;
    }

    /// the dev loop: spacewar + steam_appid.txt in the working directory. real
    ///  AppIDs never write it — their deployment is project compilation settings (S6)
    if (app_id == kSpacewarDevAppId && !std::filesystem::exists("steam_appid.txt")) {
      std::ofstream("steam_appid.txt") << app_id;
    }

    SteamErrMsg error{};
    const ESteamAPIInitResult result = SteamAPI_InitEx(&error);
    if (result != k_ESteamAPIInitResult_OK) {
      CORE_LOG_WARN("Steam unavailable ({}): {}", static_cast<int>(result), error);
      current_state = steam_state::UNAVAILABLE;
      return current_state;
    }

    steam_id = SteamUser()->GetSteamID().ConvertToUint64();
    persona = SteamFriends()->GetPersonaName();
    /// SDR warm-up so the first P2P dial doesn't pay the route-discovery latency
    SteamNetworkingUtils()->InitRelayNetworkAccess();

    current_state = steam_state::READY;
    CORE_LOG_INFO("Steam ready: {} ({:#x})", persona, steam_id);
    return current_state;
  }

  void steam_context::shutdown() {
    if (current_state == steam_state::READY) {
      SteamAPI_Shutdown();
    }
    current_state = steam_state::DISABLED;
    steam_id = 0;
    persona.clear();
  }

  void steam_context::pump() {
    if (current_state == steam_state::READY) {
      PROFILE_SECTION("steam_context::pump");
      SteamAPI_RunCallbacks();
    }
  }

}  // namespace other

/**
 * \file steam-client/steam_client.cpp
 **/
#include "steam-client/steam_client.hpp"

#include <steam/steam_api.h>

namespace other {

  void steam_client::initialize_steam_api() {
    bool require_steam = environment_cfg.get_project_value("steam-api", "require-steam");

    if (!(SteamAPI_Init())) {
      CORE_LOG_ERROR("Failed to initialize Steam API");
      state.initialized = false;
      return;
    } else {
      CORE_LOG_INFO("Successfully initialized Steam API");
      state.initialized = true;
    }

    // state.requires_restart = SteamAPI_RestartAppIfNecessary(app_id);
  }

  void steam_client::shutdown_steam_api() {
    if (state.initialized) {
      SteamAPI_Shutdown();
    }
  }

}  // namespace other
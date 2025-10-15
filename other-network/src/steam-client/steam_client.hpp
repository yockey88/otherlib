/**
 * \file  steam-client/steam_client.hpp
 **/
#ifndef OTHER_NETWORK_STEAM_CLIENT_STEAM_CLIENT_HPP
#define OTHER_NETWORK_STEAM_CLIENT_STEAM_CLIENT_HPP

#include <cstdint>

#include "core/config_table.hpp"

namespace other {

  // class SteamApi

  class steam_client {
   public:
    steam_client(const config_table& config)
        : environment_cfg(config) {}
    ~steam_client() {}

    void initialize_steam_api();
    void shutdown_steam_api();

   private:
    const config_table& environment_cfg;

    struct api_state {
      bool initialized = false;
      bool requires_restart = false;
    };
    api_state state;
  };

}  // namespace other

#endif  // OTHER_NETWORK_STEAM_CLIENT_STEAM_CLIENT_HPP
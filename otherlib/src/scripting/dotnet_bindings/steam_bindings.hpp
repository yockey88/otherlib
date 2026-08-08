/**
 * \file scripting/dotnet_bindings/steam_bindings.hpp
 **/
#ifndef OTHERLIB_SCRIPTING_DOTNET_BINDINGS_STEAM_BINDINGS_HPP
#define OTHERLIB_SCRIPTING_DOTNET_BINDINGS_STEAM_BINDINGS_HPP

#include "dotnet/native_string.hpp"
#include "dotnet/types.hpp"

namespace other {
  namespace bindings {

    nbool32 native_steam_is_available();
    uint64_t native_steam_player_id();
    native_string native_steam_player_name();
    void native_steam_open_invite_dialog();

    nbool32 native_network_host_steam();
    nbool32 native_network_join_lobby(uint64_t lobby_id);

  }  // namespace bindings
}  // namespace other

#endif  // OTHERLIB_SCRIPTING_DOTNET_BINDINGS_STEAM_BINDINGS_HPP

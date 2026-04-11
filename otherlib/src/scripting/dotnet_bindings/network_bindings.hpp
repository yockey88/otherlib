/**
 * \file scripting/dotnet_bindings/network_bindings.hpp
 **/
#ifndef OTHERLIB_SCRIPTING_DOTNET_BINDINGS_NETWORK_BINDINGS_HPP
#define OTHERLIB_SCRIPTING_DOTNET_BINDINGS_NETWORK_BINDINGS_HPP

#include "dotnet/types.hpp"

namespace other {
  namespace bindings {

    nbool32 native_network_is_connected();
    int32_t native_network_get_role();

  }  // namespace bindings
}  // namespace other

#endif  // OTHERLIB_SCRIPTING_DOTNET_BINDINGS_NETWORK_BINDINGS_HPP
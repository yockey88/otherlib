/**
 * \file scripting/dotnet_bindings/network_bindings.cpp
 **/
#include "scripting/dotnet_bindings/network_bindings.hpp"

namespace other {
  namespace bindings {

    nbool32 native_network_is_connected() {
      /// \todo connect to network subsystem
      return false;
    }

    int32_t native_network_get_role() {
      /// \todo connect to driver role
      return 0;
    }

  }  // namespace bindings
}  // namespace other
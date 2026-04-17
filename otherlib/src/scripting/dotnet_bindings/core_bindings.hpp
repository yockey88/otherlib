/**
 * \file scripting/dotnet_bindings/core_bindings.hpp
 **/
#ifndef OTHER_ENVIRONMENT_SCRIPTING_DOTNET_BINDINGS_CORE_BINDINGS_HPP
#define OTHER_ENVIRONMENT_SCRIPTING_DOTNET_BINDINGS_CORE_BINDINGS_HPP

#include <cstdint>

#include "dotnet/native_string.hpp"

namespace other {
  namespace bindings {

    uint64_t native_oe_fnv_hash(native_string str);

  }  // namespace bindings
}  // namespace other

#endif  // OTHER_ENVIRONMENT_SCRIPTING_DOTNET_BINDINGS_CORE_BINDINGS_HPP

/**
 * \file scripting/dotnet_bindings/core_bindings.hpp
 **/
#ifndef OTHERLIB_SCRIPTING_DOTNET_BINDINGS_CORE_BINDINGS_HPP
#define OTHERLIB_SCRIPTING_DOTNET_BINDINGS_CORE_BINDINGS_HPP

#include <cstdint>

#include "dotnet/native_string.hpp"

namespace other {
  namespace bindings {

    uint64_t native_oe_fnv_hash(native_string str);

  }  // namespace bindings
}  // namespace other

#endif  // OTHERLIB_SCRIPTING_DOTNET_BINDINGS_CORE_BINDINGS_HPP

/**
 * \file scripting/dotnet_bindings/core_bindings.cpp
 **/
#include "scripting/dotnet_bindings/core_bindings.hpp"

#include <string>

#include "core/fnv.hpp"

namespace other {
  namespace bindings {

    uint64_t native_oe_fnv_hash(native_string str) {
      std::string s = str;
      return FNV(s);
    }

  }  // namespace bindings
}  // namespace other

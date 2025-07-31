/**
 * \file dotnet/dotnet_field.cpp
 **/
#include "dotnet/dotnet_field.hpp"

#include "core/arena.hpp"
#include "core/defines.hpp"
#include "core/logger.hpp"

#include "dotnet/host.hpp"
#include "dotnet/native_string.hpp"

namespace other {

  std::string dotnet_field::name() const {
    OTHER_ASSERT(host != nullptr, "dotnet_host is null");
    native_string name_str;
    if (flags.is_property) {
      name_str = host->interop().get_property_name(dotnet_id);
    } else {
      name_str = host->interop().get_field_name(dotnet_id);
    }
    std::string res = name_str;
    native_string::free_str(name_str);
    return res;
  }

}  // namespace other
/**
 * \file dotnet/dotnet_method.cpp
 **/
#include "dotnet/dotnet_method.hpp"

#include "core/logger.hpp"

#include "dotnet/host.hpp"

namespace other {

  dotnet_type* dotnet_method::get_return_type() {
    OTHER_ASSERT(host != nullptr, "dotnet_host is null");
    if (return_type == nullptr) {
      int32_t return_type_id = -1;
      host->interop().get_method_return_type(dotnet_id, &return_type_id);
      return_type = host->get_type_cache()->cache_type(host, return_type_id);
    }
    return return_type;
  }

  std::string dotnet_method::name() const {
    native_string name_str = host->interop().get_method_name(dotnet_id);
    std::string res = name_str;
    native_string::free_str(name_str);
    return res;
  }

}  // namespace other
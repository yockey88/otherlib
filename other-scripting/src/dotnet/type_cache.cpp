/**
 * \file dotnet/type_cache.cpp
 **/
#include "dotnet/type_cache.hpp"

#include "dotnet/host.hpp"

namespace other {

  dotnet_type* type_cache::cache_type(dotnet_host* host, int32_t dotnet_id) {
    auto [itr, success] = cached_types.insert({ dotnet_id, dotnet_type(host, dotnet_id) });
    if (!success) {
      CORE_LOG_ERROR("Failed to cache .NET type [{}]", dotnet_id);
    }

    itr->second.initialize_type_interface();
    return &itr->second;
  }

}  // namespace other
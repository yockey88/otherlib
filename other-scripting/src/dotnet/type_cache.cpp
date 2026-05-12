/**
 * \file dotnet/type_cache.cpp
 **/
#include "dotnet/type_cache.hpp"

#include "core/fnv.hpp"

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

  void type_cache::remove_type(int32_t dotnet_handle) {
    auto itr = cached_types.find(dotnet_handle);
    if (itr == cached_types.end()) {
      CORE_LOG_ERROR("Failed to remove .NET type [{}] from cache: not found", dotnet_handle);
      return;
    }

    name_cache.erase(FNV(itr->second.full_name()));
    id_cache.erase(dotnet_handle);
    cached_types.erase(itr);
  }

  void type_cache::clear_cache(dotnet_host* host) {
    cached_types.clear();
    name_cache.clear();
    id_cache.clear();
  }

  dotnet_type* type_cache::get_type(const std::string_view name) {
    bool contains = name_cache.contains(FNV(name));
    dotnet_type* res = nullptr;

    if (!contains) {
      for (auto itr = cached_types.begin(); itr != cached_types.end(); ++itr) {
        if (itr->second.full_name() == name) {
          name_cache[FNV(name)] = &itr->second;
          res = &itr->second;
        }
      }
    } else {
      res = name_cache[FNV(name)];
    }

    return res;
  }

  dotnet_type* type_cache::get_type(int32_t id) {
    bool contains = id_cache.contains(id);
    dotnet_type* res = nullptr;
    if (!contains) {
      auto itr = cached_types.find(id);
      if (itr != cached_types.end()) {
        res = &itr->second;
        id_cache[id] = res;
      }
    } else {
      res = id_cache[id];
    }
    return res;
  }

}  // namespace other
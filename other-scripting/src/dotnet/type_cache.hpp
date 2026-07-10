/**
 * \file dotnet/type_cache.hpp
 **/
#ifndef OTHER_ENGINE_SCRIPTING_DOTNET_TYPE_CACHE_HPP
#define OTHER_ENGINE_SCRIPTING_DOTNET_TYPE_CACHE_HPP

#include <cstdint>

#include "core/memory_pool.hpp"
#include "core/ref.hpp"

#include "dotnet/dotnet_type.hpp"

namespace other {

  class dotnet_host;

  class type_cache {
   public:
    type_cache() = default;
    ~type_cache() = default;

    dotnet_type* cache_type(dotnet_host* host, int32_t dotnet_handle);
    void remove_type(int32_t dotnet_handle);

    void clear_cache(dotnet_host* interop);

    dotnet_type* get_type(const std::string_view name);
    dotnet_type* get_type(int32_t id);

    [[nodiscard]] auto begin() { return cached_types.begin(); }
    [[nodiscard]] auto end() { return cached_types.end(); }
    [[nodiscard]] auto rbegin() { return cached_types.rbegin(); }
    [[nodiscard]] auto rend() { return cached_types.rend(); }

    [[nodiscard]] auto begin() const { return cached_types.begin(); }
    [[nodiscard]] auto end() const { return cached_types.end(); }
    [[nodiscard]] auto rbegin() const { return cached_types.rbegin(); }
    [[nodiscard]] auto rend() const { return cached_types.rend(); }

    auto size() const { return cached_types.size(); }

   private:
    ostd::map<int32_t, dotnet_type> cached_types;

    ostd::unordered_map<uint64_t, dotnet_type*> name_cache;
    ostd::unordered_map<int32_t, dotnet_type*> id_cache;
  };

}  // namespace other

#endif  // OTHER_ENGINE_SCRIPTING_DOTNET_TYPE_CACHE_HPP
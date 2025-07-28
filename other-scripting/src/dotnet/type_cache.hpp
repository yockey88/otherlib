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

   private:
    std::map<int32_t, dotnet_type> cached_types;
  };

}  // namespace other

#endif  // OTHER_ENGINE_SCRIPTING_DOTNET_TYPE_CACHE_HPP
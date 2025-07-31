/**
 * \file dotnet/garbage_collector.hpp
 **/
#ifndef OTHER_SCRIPTING_DOTNET_GARBAGE_COLLECTOR_HPP
#define OTHER_SCRIPTING_DOTNET_GARBAGE_COLLECTOR_HPP

#include "core/defines.hpp"

namespace other {

  enum class gc_mode : uint32_t {
    DEFAULT = 0,
    FORCED,
    OPTIMIZED,
    AGGRESSIVE,
  };

  // DEFAULT = 0,
  // GENERATION_0 = 1,
  // GENERATION_1 = 2,
  // GENERATION_2 = 3,
  // LARGE_OBJECT_HEAP = 4,

}  // namespace other

#endif  // OTHER_SCRIPTING_DOTNET_GARBAGE_COLLECTOR_HPP
/**
 * \file dotnet/dotnet_memory.hpp
 **/
#ifndef OTHER_SCRIPTING_DOTNET_DOTNET_MEMORY_HPP
#define OTHER_SCRIPTING_DOTNET_DOTNET_MEMORY_HPP

#include <string_view>

#include <dotnet/nethost.h>

namespace other {

  class dotnet_memory {
   public:
    static void* alloc_global(size_t size);
    static void free_global(void* ptr);

    static char_t* native_string_to_co_task_mem(const std::basic_string_view<char_t> str);

    static void* alloc_co_task_mem(size_t size);
    static void free_co_task_mem(void* str);
  };

}  // namespace other

#endif  // OTHER_SCRIPTING_DOTNET_DOTNET_MEMORY_HPP
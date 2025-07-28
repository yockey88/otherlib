/**
 * \file dotnet/dotnet_memory.cpp
 **/
#include "dotnet/dotnet_memory.hpp"

#include "core/defines.hpp"

#ifdef OTHER_ENVIRONMENT_WINDOWS
  #include <ShlObj_core.h>
  #include <Windows.h>

#else
#endif

namespace other {

  void* dotnet_memory::alloc_global(size_t size) {
#ifdef OTHER_ENVIRONMENT_WINDOWS
    return LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, size);
#else
    return malloc(size);
#endif  // OTHER_ENVIRONMENT_WINDOWS
  }

  void dotnet_memory::free_global(void* ptr) {
#ifdef OTHER_ENVIRONMENT_WINDOWS
    LocalFree(ptr);
#else
    free(ptr);
#endif  // OTHER_ENVIRONMENT_WINDOWS
  }

  char_t* dotnet_memory::native_string_to_co_task_mem(const std::basic_string_view<char_t> str) {
    size_t length = str.length() + 1;
    size_t size = length * sizeof(char_t);

    char_t* buffer = static_cast<char_t*>(alloc_co_task_mem(size));

    if (buffer != nullptr) {
#ifdef _WIN32
      memset(buffer, 0xCE, size);
      wcscpy(buffer, str.data());
#else
      memset(buffer, 0, size);
      strcpy(buffer, str.data());
#endif  // _WIN32
    }
    return buffer;
  }

  void* dotnet_memory::alloc_co_task_mem(size_t size) {
#ifdef OTHER_ENVIRONMENT_WINDOWS
    return CoTaskMemAlloc(size);
#else
    return alloc_global(size);
#endif  // OTHER_ENVIRONMENT_WINDOWS
  }

  void dotnet_memory::free_co_task_mem(void* memory) {
#ifdef OTHER_ENVIRONMENT_WINDOWS
    CoTaskMemFree(memory);
#else
    free_global(memory);
#endif  // OTHER_ENVIRONMENT_WINDOWS
  }

}  // namespace other
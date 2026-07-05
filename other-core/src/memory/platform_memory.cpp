/**
 * \file memory/platform_memory.cpp
 **/
#include "memory/platform_memory.hpp"

#ifdef OTHER_ENVIRONMENT_WINDOWS
  #include <windows.h>
#else
  #include <sys/mman.h>
#endif

#include "core/logger.hpp"

namespace other {
  namespace detail {

    void* platform_allocation(uint8_t* page_base);
    void platform_free(void* ptr);

  }  // namespace detail

  void platform_memory::initialize() {
    OTHER_ASSERT(base_ptr == nullptr, "page_bank initialized twice.");
    base_ptr = static_cast<uint8_t*>(detail::platform_allocation(nullptr));
    OTHER_ASSERT(base_ptr != nullptr, "Failed to reserve {} bytes of address space.", kReservation);
  }

  void platform_memory::shutdown() {
    if (base_ptr == nullptr) {
      return;
    }
  }

  uint8_t* platform_memory::create_new_page() {
    std::lock_guard lock(platform_memory_mutex);

    if (next_page >= kMaxPages) {
      CORE_LOG_ERROR("Maximum page limit of {} reached. Cannot allocate more pages.", kMaxPages);
      return nullptr;
    }

    uint8_t* page_base = page_at(next_page);
    void* result = detail::platform_allocation(page_base);
    if (result == nullptr) {
      CORE_LOG_ERROR("Failed to commit memory for page {} at address {:p}.", next_page, static_cast<void*>(page_base));
      return nullptr;
    }

    ++next_page;
    return static_cast<uint8_t*>(result);
  }

  namespace detail {

    void* platform_allocation(uint8_t* page_base) {
      return
#ifdef OTHER_ENVIRONMENT_WINDOWS
        VirtualAlloc(page_base, platform_memory::kReservation, MEM_RESERVE, PAGE_READWRITE);
#else
        void* mem = mmap(page_base, platform_memory::kReservation, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
      (mem == MAP_FAILED) ? nullptr : static_cast<uint8_t*>(mem)
#endif
      ;
    }

    void platform_free(void* ptr) {
#ifdef OTHER_ENVIRONMENT_WINDOWS
      VirtualFree(ptr, 0, MEM_RELEASE);
#else
      munmap(ptr, platform_memory::kReservation);
#endif
    }

  }  // namespace detail
}  // namespace other
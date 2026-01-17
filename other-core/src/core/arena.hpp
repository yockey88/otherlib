/**
 * \file core/arena.hpp
 **/
#ifndef OTHER_CORE_MEMORY_ARENA_HPP
#define OTHER_CORE_MEMORY_ARENA_HPP

#include <cstdint>
#ifndef OTHERENV_WINDOWS
  #include <cstddef>
#endif
#include <mutex>

#include "core/profiler.hpp"
#include "core/subsystem.hpp"

namespace other {

  struct arena_storage {
    static inline constexpr size_t kPageSize = 2 * 64 * 4096u;  // 128 MB
    static inline constexpr size_t kAlignment = 16;
    static inline constexpr size_t kMaxPages = 64;
    static inline constexpr size_t kMaxMemoryAllowed = kMaxPages * kPageSize;

    struct page {
      size_t cursor = 0;
      /// \todo this alignment is not not correct for all types and causes
      ///       issue with things like asio::io_context (if we don't include it here it will be 1 byte aligned)
      alignas(kAlignment) uint8_t storage[kPageSize] = {};

      void* data() { return &storage[0]; }
      const void* data() const { return &storage[0]; }

      void* get_ptr_at(size_t offset);
    };

    page* allocate_page(size_t idx);
    void free_page(size_t index);

    page* get_page(size_t idx);

   private:
#ifdef OTHER_TEST_ENVIRONMENT
    friend class arena_test;
#endif
    page* pages[kMaxPages];
  };

  class arena : public subsystem<arena> {
   public:
    using page = arena_storage::page;

    arena() = default;
    ~arena();

    static void* allocate(size_t size, size_t alignment = arena_storage::kAlignment);
    static void free(void* ptr, size_t size);
    static void free(void* ptr);

    void* request_region(size_t size, size_t alignment = arena_storage::kAlignment);
    void free_region(void* ptr);

    page* get_current_page();

   private:
    PROFILE_MUTEX_TYPE(std::mutex, arena_mutex);
    page* current_page = nullptr;

   private:
    arena_storage storage;
    size_t page_allocation_cursor = 0;
    size_t total_allocations = 0;
    size_t allocated_memory = 0;

    size_t live_allocations = 0;

    void allocate_page();

#ifdef OTHER_TEST_ENVIRONMENT
    friend class arena_test;
#endif
    // #ifdef OTHER_MEMORY_DEBUG
    //     static void report_allocation(void* ptr, std::size_t size);
    // #endif
  };

}  // namespace other

OTHER_SUBSYSTEM(other::arena);

#endif  // OTHER_CORE_MEMORY_ARENA_HPP

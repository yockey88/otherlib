/**
 * \file memory/arena.hpp
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
#include "memory/arena_storage.hpp"
#include "memory/free_list.hpp"
#include "memory/page.hpp"

namespace other {

  struct page;

  struct alloc_header {
    uint32_t user_size;  // caller requested size
    uint8_t bin;
    uint8_t flags;
    uint16_t magic;
    uint64_t reserved;  // padded size (padded to 16 bytes)
  };
  static_assert(sizeof(alloc_header) == 16, "Header must preserve 16-byte user alignment.");
  static_assert(sizeof(alloc_header) == page::kAlignment, "user pointer = block + header must stay aligned.");
  static_assert(sizeof(alloc_header) == free_list::kMinBlockSize / 2, "free_node must exactly overlay alloc_header (see design-c-free-list §1).");
  static_assert(free_list::kMinBlockSize == 2 * page::kAlignment, "blocks must advance cursors in 2*alignment steps.");

  class arena : public subsystem<arena> {
   public:
    constexpr static inline size_t kHeaderSize = sizeof(alloc_header);

    arena() = default;
    ~arena();

    static void* allocate(size_t size, size_t alignment = page::kAlignment);
    static void free(void* ptr, size_t size);
    static void free(void* ptr);

    void* request_region(size_t size, size_t alignment = page::kAlignment);
    void free_region(void* ptr);

    page* get_current_page();

   private:
    // unique tag to help debug memory issues
    constexpr static inline uint16_t kAllocationTag = 0xA11C;

    PROFILE_MUTEX_TYPE(std::mutex, arena_mutex);
    page* current_page = nullptr;

    arena_storage storage;
    free_list freelist;
    size_t page_allocation_cursor = 0;
    size_t total_allocations = 0;
    size_t allocated_memory = 0;
    size_t requested_memory = 0;
    size_t used_memory = 0;

    size_t live_allocations = 0;

    void* bump(size_t size);
    void handle_spillover();

    size_t get_allocation_padding(size_t size, size_t alignment) const;
    void allocate_page();

#ifdef OTHER_TEST_ENVIRONMENT
    friend class arena_test;
#endif
    // #ifdef OTHER_MEMORY_DEBUG
    //     static void report_allocation(void* ptr, std::size_t size);
    // #endif
  };

}  // namespace other

OTHER_DEPENDENT_SUBSYSTEM(
  other::arena,
  subsystem_profile::kLogger);

#endif  // OTHER_CORE_MEMORY_ARENA_HPP

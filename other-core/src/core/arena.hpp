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

#include "core/arena_storage.hpp"
#include "core/page.hpp"
#include "core/profiler.hpp"
#include "core/subsystem.hpp"

namespace other {

  struct page;

  class arena : public subsystem<arena> {
   public:
    arena() = default;
    ~arena();

    static void* allocate(size_t size, size_t alignment = page::kAlignment);
    static void free(void* ptr, size_t size);
    static void free(void* ptr);

    void* request_region(size_t size, size_t alignment = page::kAlignment);
    void free_region(void* ptr);

    static page* request_memory_page();
    static void free_memory_page(page* region);

    page* get_current_page();

   private:
    PROFILE_MUTEX_TYPE(std::mutex, arena_mutex);
    page* current_page = nullptr;

    arena_storage storage;
    // free_list free_list;
    size_t page_allocation_cursor = 0;
    size_t total_allocations = 0;
    size_t allocated_memory = 0;
    size_t used_memory = 0;

    size_t live_allocations = 0;

    size_t get_allocation_padding(size_t size, size_t alignment) const;
    size_t check_page_and_recalculate_padding(size_t size, size_t alignment, size_t space_needed);
    void* do_allocation(size_t size, size_t alignment, size_t padding, size_t final_size);
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
  subsystem_profile::kLogger,
);

#endif  // OTHER_CORE_MEMORY_ARENA_HPP

/**
 * \file core/arena.cpp
 **/
#include "core/arena.hpp"

#include <cstdint>

#include "core/arena_storage.hpp"
#include "core/logger.hpp"
#include "core/page.hpp"
#include "core/profiler.hpp"

#include "profiler.hpp"

namespace other {

  arena::~arena() {
    storage.cleanup(page_allocation_cursor);
    page_allocation_cursor = 0;
  }

  void* arena::allocate(size_t size, size_t alignment) {
    PROFILE_SECTION("arena::allocate");
    OTHER_ASSERT(size <= page::kPageSize, "Allocation size {} is too large for Arena.", size);
    arena* instance = subsystem_description<arena>::ptr();
    std::lock_guard lock_arena_mutex(instance->arena_mutex);

    size_t padding = instance->get_allocation_padding(size, alignment);

    size_t actual_space_needed = size + padding;
    padding = instance->check_page_and_recalculate_padding(size, alignment, actual_space_needed);
    return instance->do_allocation(size, alignment, padding, actual_space_needed);
  }

  void arena::free(void* ptr, std::size_t size) {
    PROFILE_SECTION("arena::free");
    arena* instance = subsystem_description<arena>::ptr();
    std::lock_guard lock_arena_mutex(instance->arena_mutex);

    instance->allocated_memory -= size;
    instance->live_allocations--;
    PROFILE_DEALLOCATION(ptr);

    /// do nothing for now, allocators handle calling destructors and zeroing memory
    ///   later we can implement a free list or something or register freed chunks for defragmentation
  }

  void arena::free(void* ptr) {
    PROFILE_SECTION("arena::free");
    arena* instance = subsystem_description<arena>::ptr();
    std::lock_guard lock_arena_mutex(instance->arena_mutex);

    //// be nice to have size info here but oh well
    instance->live_allocations--;
    PROFILE_DEALLOCATION(ptr);
  }

  void* arena::request_region(size_t size, size_t alignment) {
    PROFILE_SECTION("arena::request_region");
    std::lock_guard lock_arena_mutex(arena_mutex);

    void* region = allocate(size, alignment);
    OTHER_ASSERT(region != nullptr, "Failed to allocate aligned region.");
    return region;
  }

  void arena::free_region(void* ptr) {
    PROFILE_SECTION("arena::free_region");
    std::lock_guard lock_arena_mutex(arena_mutex);
    this->free(ptr);
  }

  page* arena::request_memory_page() {
    arena* instance = subsystem_description<arena>::ptr();
    std::lock_guard lock_arena_mutex(instance->arena_mutex);
    return instance->storage.create_page();
  }

  void arena::free_memory_page(page* region) {
    arena* instance = subsystem_description<arena>::ptr();
    std::lock_guard lock_arena_mutex(instance->arena_mutex);
    instance->storage.free_page(region);
  }

  page* arena::get_current_page() {
    arena* instance = subsystem_description<arena>::ptr();
    std::lock_guard lock_arena_mutex(instance->arena_mutex);
    return current_page;
  }

  size_t arena::get_allocation_padding(size_t size, size_t alignment) const {
    size_t alignment_shift = 0;
    if (current_page != nullptr) {
      alignment_shift = current_page->cursor % alignment;
      if (alignment_shift != 0) {
        return alignment - alignment_shift;
      }
    }
    return 0;
  }

  size_t arena::check_page_and_recalculate_padding(size_t size, size_t alignment, size_t space_needed) {
    bool has_no_page = page_allocation_cursor == 0 || current_page == nullptr;
    bool out_of_space = current_page != nullptr && (current_page->cursor + space_needed >= page::kPageSize);
    if (has_no_page || out_of_space) {
      OTHER_ASSERT(page_allocation_cursor < arena_storage::kMaxPages, "Exceeded maximum number of pages. Allocating page : {}.", page_allocation_cursor);
      allocate_page();
    }
    OTHER_ASSERT(current_page != nullptr, "Current page is null.");

    size_t padding = 0;
    size_t alignment_shift = current_page->cursor % alignment;
    if (alignment_shift != 0) {
      padding = alignment - alignment_shift;
    }
    return padding;
  }

  void* arena::do_allocation(size_t size, size_t alignment, size_t padding, size_t final_size) {
    current_page->cursor += padding;

    void* mem = nullptr;
    {
      PROFILE_SECTION("arena::allocate--perform-allocation");
      mem = current_page->get_ptr_at(current_page->cursor);
      OTHER_ASSERT(mem != nullptr, "Failed to get pointer from current page.");
    }
    PROFILE_ALLOCATION(mem, size);

    current_page->cursor += size;
    used_memory += size;
    allocated_memory += final_size;
    total_allocations++;
    live_allocations++;
    return mem;
  }

  void arena::allocate_page() {
    OTHER_ASSERT(page_allocation_cursor < arena_storage::kMaxPages, "Exceeded maximum number of pages. Allocating page : {}.", page_allocation_cursor);
    PROFILE_SECTION("arena::allocate_page");

    if (current_page != nullptr) {
      CORE_LOG_TRACE("[ARENA] Finalized page allocation with {} bytes used.", current_page->cursor);
      CORE_LOG_TRACE("[ARENA] Total allocations so far: {}, total allocated memory: {} bytes.", total_allocations, allocated_memory);
      CORE_LOG_TRACE("[ARENA] Live allocations: {}.", live_allocations);
    }

    current_page = storage.allocate_page(page_allocation_cursor);
    OTHER_ASSERT(current_page != nullptr, "Failed to allocate page.");
    page_allocation_cursor++;

    CORE_LOG_TRACE("[ARENA] Allocated new page ({} of {})", page_allocation_cursor, arena_storage::kMaxPages);
  }

}  // namespace other
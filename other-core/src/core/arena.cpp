/**
 * \file core/arena.cpp
 **/
#include "core/arena.hpp"

#include <cstdint>

#include "core/logger.hpp"
#include "core/profiler.hpp"

#include "profiler.hpp"

namespace other {

  void* arena_storage::page::get_ptr_at(size_t offset) {
    OTHER_ASSERT(offset < kPageSize, "Offset out of bounds for page allocation.");
    return &storage[offset];
  }

  arena_storage::page* arena_storage::allocate_page(size_t idx) {
    OTHER_ASSERT(idx < kMaxPages, "Index out of bounds for page allocation.");
    pages[idx] = new page();
    std::memset(pages[idx]->data(), 0, kPageSize);
    return pages[idx];
  }

  void arena_storage::free_page(size_t index) {
    OTHER_ASSERT(index < kMaxPages, "Index out of bounds for page allocation.");
    delete pages[index];
    pages[index] = nullptr;
  }

  arena_storage::page* arena_storage::get_page(size_t idx) {
    if (idx >= kMaxPages || pages[idx] == nullptr) {
      return nullptr;
    }
    return pages[idx];
  }

  arena::~arena() {
    for (size_t i = 0; i < page_allocation_cursor; i++) {
      storage.free_page(i);
    }
    page_allocation_cursor = 0;
  }

  void* arena::allocate(size_t size) {
    PROFILE_SECTION("arena::allocate");
    OTHER_ASSERT(size <= arena_storage::kPageSize, "Allocation size is too large for Arena.");
    static arena* instance = subsystem_description<arena>::ptr();
    std::lock_guard lock(instance->arena_mutex);

    if (instance->page_allocation_cursor == 0 ||
        instance->current_page == nullptr || instance->current_page->cursor + size >= arena_storage::kPageSize) {
      OTHER_ASSERT(instance->page_allocation_cursor < arena_storage::kMaxPages, "Exceeded maximum number of pages. Allocating page : {}.", instance->page_allocation_cursor);
      instance->allocate_page();
    }
    OTHER_ASSERT(instance->current_page != nullptr, "Current page is null.");

    void* mem = nullptr;
    {
      PROFILE_SECTION("arena::allocate--perform-allocation");
      mem = instance->current_page->get_ptr_at(instance->current_page->cursor);
    }
    PROFILE_ALLOCATION(mem, size);

    OTHER_ASSERT(mem != nullptr, "Failed to get pointer from current page.");
    instance->current_page->cursor += size;
    instance->allocated_memory += size;
    instance->total_allocations++;
    instance->live_allocations++;

    // #ifdef OTHERENV_MEMORY_DEBUG
    //     ReportAllocation(mem, size);
    // #endif
    return mem;
  }

  void* arena::request_region(size_t size, size_t alignment) {
    PROFILE_SECTION("arena::request_region");
    void* ptr = malloc(size + alignment - 1);
    OTHER_ASSERT(ptr != nullptr, "Failed to allocate memory region of size {} with alignment {}.", size, alignment);
    uintptr_t addr = reinterpret_cast<uintptr_t>(ptr);
    uintptr_t aligned_addr = (addr + alignment - 1) & ~(alignment - 1);
    return reinterpret_cast<void*>(aligned_addr);
  }

  void arena::free(void* ptr, std::size_t size) {
    PROFILE_SECTION("arena::free");
    static arena* instance = subsystem_description<arena>::ptr();
    std::lock_guard lock(instance->arena_mutex);

    instance->allocated_memory -= size;
    instance->live_allocations--;
    /// do nothing for now, allocators handle calling destructors and zeroing memory
    ///   later we can implement a free list or something or register freed chunks for defragmentation
    PROFILE_DEALLOCATION(ptr);
  }

  void arena::free_region(void* ptr) {
    PROFILE_SECTION("arena::free_region");
    OTHER_ASSERT(ptr != nullptr, "Cannot free a null pointer.");
    ::free(ptr);
  }

  arena::page* arena::get_current_page() {
    return current_page;
  }

  void arena::allocate_page() {
    OTHER_ASSERT(page_allocation_cursor < arena_storage::kMaxPages, "Exceeded maximum number of pages. Allocating page : {}.", page_allocation_cursor);

    current_page = storage.allocate_page(page_allocation_cursor++);
    OTHER_ASSERT(current_page != nullptr, "Failed to allocate page.");
    current_page->cursor = 0;
  }

  // #ifdef OTHERENV_MEMORY_DEBUG
  //   void Arena::ReportAllocation(void* addr, std::size_t sz) {
  //     std::stringstream ss;
  //     ss << "Allocated memory at address: " << addr << ", size: " << sz << "\n";
  //     ss << "   > Total Allocations: " << total_allocations << "\n";
  //     ss << "   > Allocated Memory: " << allocated_memory << "\n";
  //     ss << "   > Page Allocation Cursor: " << page_allocation_cursor << "\n";
  //     ss << "   > Page Cursor: " << page_cursor << "\n";
  //     // OE_DEBUG(ss.str());
  //   }
  // #endif

}  // namespace other
/**
 * \file core/arena.cpp
 **/
#include "core/arena.hpp"

#include <cstdint>

// #include <new>

namespace other {

  arena::~arena() {
    // PROFILE_SECTION("Arena--Destructor");

    for (size_t i = 0; i < page_allocation_cursor; i++) {
      // PROFILE_DEALLOCATION(pages[i]);
      std::free(pages[i]);
      pages[i] = nullptr;
    }
    page_allocation_cursor = 0;
    page_cursor = 0;
  }

  void* arena::allocate(size_t size, size_t alignment) {
    //     OE_ASSERT(instance != nullptr, "Arena instance is null.");
    //     OE_ASSERT(size <= kPageSize, "Allocation size is too large for Arena.");

    //     PROFILE_SECTION("Arena--Allocate");

    if (instance->page_allocation_cursor == 0 || instance->page_cursor + size >= kPageSize) {
      instance->allocate_page();
    }
    //     OE_ASSERT(instance->page_allocation_cursor < kMaxPages, "Exceeded maximum number of pages.");

    /// FIXME: this alignment offset may not be correct for all cases because the alignment is hardcoded to 16 bytes,
    ///         but I need to figure out how to balance the needed alignment for std containers while keeping 16 byte
    ///         alignment because GPUs read memory in 16 byte chunks
    // clang-format off
    size_t alignment_offset = (instance->page_cursor % alignment) != 0 ?
      alignment - (instance->page_cursor % alignment) : 0;
    // clang-format on
    instance->page_cursor += alignment_offset;

    void* mem = instance->pages[instance->page_allocation_cursor - 1] + instance->page_cursor;

    instance->total_allocations++;
    instance->allocated_memory += size;
    instance->page_cursor += size;

    // #ifdef OTHERENV_MEMORY_DEBUG
    //     ReportAllocation(mem, size);
    // #endif

    return mem;
  }

  void arena::free(void* ptr, std::size_t size) {
    /// do nothing for now, allocators handle calling destructors and zeroing memory
    ///   later we can implement a free list or something or register freed chunks for defragmentation
    return;
  }

  void arena::allocate_page() {
    // OE_TRACE("Attempting to page allocation.");
    // OE_ASSERT(page_allocation_cursor < kMaxPages, "Exceeded maximum number of pages. Allocating page : {}.", page_allocation_cursor);
    // OE_ASSERT(pages[page_allocation_cursor] == nullptr, "Page already allocated.");

    // PROFILE_SECTION("Arena--AllocatePage");

    pages[page_allocation_cursor++] = (uint8_t*)malloc(kPageSize);
    // OE_ASSERT(pages[page_allocation_cursor] != nullptr, "Failed to allocate page.");

    page_cursor = 0;
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
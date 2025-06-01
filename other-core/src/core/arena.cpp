/**
 * \file core/arena.cpp
 **/
#include "core/arena.hpp"

#include <cstdint>

#include "core/logger.hpp"

namespace other {

  void* arena_storage::page::get_ptr_at(size_t offset) {
    OTHER_ASSERT(offset < kPageSize, "Offset out of bounds for page allocation.");
    return &storage[offset];
  }

  arena_storage::page* arena_storage::allocate_page(size_t idx) {
    OTHER_ASSERT(idx < kMaxPages, "Page index out of bounds.");

    /// PROFILE_SECTION("ArenaStorage--AllocatePage");
    pages[idx] = new page();
    std::memset(pages[idx]->storage, 0, kPageSize);
    OTHER_ASSERT(pages[idx] != nullptr, "Failed to allocate page.");
    return pages[idx];
  }

  void arena_storage::free_page(size_t index) {
    OTHER_ASSERT(index < kMaxPages, "Page index out of bounds.");
    OTHER_ASSERT(pages[index] != nullptr, "Page is already freed or not allocated.");

    // PROFILE_DEALLOCATION(pages[index]);
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
    // PROFILE_SECTION("Arena--Destructor");
    for (size_t i = 0; i < page_allocation_cursor; i++) {
      storage.free_page(i);
    }
    page_allocation_cursor = 0;
  }

  void* arena::allocate(size_t size) {
    arena* instance = subsystem<arena>::get();

    OTHER_ASSERT(instance != nullptr, "Arena instance is null.");
    OTHER_ASSERT(size <= arena_storage::kPageSize, "Allocation size is too large for Arena.");

    //     PROFILE_SECTION("Arena--Allocate");
    void* mem = nullptr;
    {
      std::lock_guard lock(instance->mtx);

      if (instance->page_allocation_cursor == 0) {
        instance->allocate_page();
        OTHER_ASSERT(instance->get_current_page() != nullptr, "Failed to allocate initial page.");
      }

      page* current_page = instance->get_current_page();
      if (current_page->cursor + size >= arena_storage::kPageSize) {
        OTHER_ASSERT(instance->page_allocation_cursor < arena_storage::kMaxPages, "Exceeded maximum number of pages.");
        instance->allocate_page();
        current_page = instance->get_current_page();
      }
      OTHER_ASSERT(current_page != nullptr, "Current page is null.");

      /// TODO: investigate if this alignment is wrong or not if feels safe enought but what do I know
      // clang-format off
      size_t alignment_offset = (current_page->cursor % arena_storage::kAlignment) != 0 ?
          arena_storage::kAlignment - (current_page->cursor % arena_storage::kAlignment) : 0;
      // clang-format on
      current_page->cursor += alignment_offset;

      mem = current_page->get_ptr_at(current_page->cursor);

      instance->total_allocations++;
      instance->allocated_memory += size;
      current_page->cursor += size;
    }
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

  arena::page* arena::get_current_page() {
    return storage.get_page(page_allocation_cursor - 1);
  }

  void arena::allocate_page() {
    // PROFILE_SECTION("Arena--AllocatePage");
    OTHER_ASSERT(page_allocation_cursor < arena_storage::kMaxPages, "Exceeded maximum number of pages. Allocating page : {}.", page_allocation_cursor);

    page* p = storage.allocate_page(page_allocation_cursor++);
    OTHER_ASSERT(p != nullptr, "Failed to allocate page.");

    p->cursor = 0;
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
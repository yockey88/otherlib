/**
 * \file core/arena.cpp
 **/
#include "memory/arena.hpp"

#include <cstdint>

#include "core/logger.hpp"
#include "core/profiler.hpp"
#include "memory/arena_storage.hpp"
#include "memory/page.hpp"

#if 1
  #define USE_NEW_ALLOCATION
#endif

namespace other {

  arena::~arena() {
    freelist.cleanup();
    storage.cleanup(page_allocation_cursor);
    storage.finalize();
    page_allocation_cursor = 0;
  }

  void* arena::allocate(size_t size, size_t alignment) {
    PROFILE_SECTION("arena::allocate");
    OTHER_ASSERT(size > 0, "arena::allocate called with size 0.");
    OTHER_ASSERT(alignment <= page::kAlignment, "arena supports alignment <= {} (requested {}).", page::kAlignment, alignment);
    OTHER_ASSERT(size + sizeof(alloc_header) <= free_list::bin_block_size(free_list::kNumBins - 1), "Allocation size {} exceeds arena maximum.", size);

    auto& instance = instance_ref();
    std::lock_guard lock(instance.arena_mutex);

    const size_t bin = free_list::bin_index(size + sizeof(alloc_header));
    void* block = nullptr;
    {
      block = instance.freelist.pop(bin);
      if (block == nullptr) {
        block = instance.bump(free_list::bin_block_size(bin));
      }
      instance.live_allocations++;
      instance.total_allocations++;
      instance.requested_memory += size;
      instance.used_memory += free_list::bin_block_size(bin);
    }

    auto* header = static_cast<alloc_header*>(block);
    header->user_size = static_cast<uint32_t>(size);
    header->bin = static_cast<uint8_t>(bin);
    header->flags = 0;
    header->magic = kAllocationTag;
    PROFILE_ALLOCATION(header + 1, size);

    void* mem = header + 1;
    OTHER_ASSERT((reinterpret_cast<uintptr_t>(mem) & (page::kAlignment - 1)) == 0, "arena produced a misaligned pointer {:p}.", mem);
    return mem;
  }

  void arena::free(void* ptr, std::size_t size) {
    PROFILE_SECTION("arena::free");
    if (ptr != nullptr) {
      auto* header = static_cast<alloc_header*>(ptr) - 1;
      OTHER_ASSERT(header->magic == kAllocationTag, "arena::free called on a pointer the arena does not own.");
      OTHER_ASSERT(header->user_size == size, "arena::free size mismatch: allocated {}, freed {}.", header->user_size, size);
    }
    free(ptr);
  }

  void arena::free(void* ptr) {
    PROFILE_SECTION("arena::free");
    if (ptr == nullptr) {
      return;
    }
    auto* header = static_cast<alloc_header*>(ptr) - 1;
    OTHER_ASSERT(header->magic == kAllocationTag, "arena::free called on a pointer the arena does not own.");
    PROFILE_DEALLOCATION(ptr);

    auto& instance = instance_ref();
    std::lock_guard lock_arena_mutex(instance.arena_mutex);
    instance.freelist.push(header->bin, header);
    instance.live_allocations--;
    instance.used_memory -= free_list::bin_block_size(header->bin);
  }

  page* arena::request_memory_page() {
    auto& instance = instance_ref();
    std::lock_guard lock(instance.arena_mutex);
    OTHER_ASSERT(instance.page_allocation_cursor < arena_storage::kMaxPages, "Exceeded maximum number of pages. Allocating page : {}.", instance.page_allocation_cursor);
    return instance.storage.create_page();
  }

  void arena::free_memory_page(page* p) {
    auto& instance = instance_ref();
    std::lock_guard lock(instance.arena_mutex);
    instance.storage.destroy_page(p);
  }

  frame_allocator* arena::create_frame_allocator() {
    auto& instance = instance_ref();
    std::lock_guard lock(instance.arena_mutex);
    return instance.storage.create_frame_allocator();
  }

  void arena::destroy_frame_allocator(frame_allocator* frame) {
    auto& instance = instance_ref();
    std::lock_guard lock(instance.arena_mutex);
    instance.storage.destroy_frame_allocator(frame);
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
    get_instance().free(ptr);
  }

  page* arena::get_current_page() {
    return instance_ref().current_page;
  }

  void* arena::bump(size_t size) {
    if (current_page == nullptr || current_page->cursor + size > page::kPageSize) {
      handle_spillover();
      allocate_page();
    }

    void* mem = current_page->get_ptr_at(current_page->cursor);
    current_page->cursor += size;

    return mem;
  }

  void arena::handle_spillover() {
    /// caller holds arena_mutex
    // decompose the tail of outgoing page into pow2 blocks and assign them to a bin to avoid too much fragmentation
    if (current_page == nullptr) {
      return;
    }

    size_t cursor = current_page->cursor;
    while (page::kPageSize - cursor >= 32) {
      /// largest pow2 block that fits both the remaining space and the cursor's own alignment
      const size_t space = page::kPageSize - cursor;
      size_t block = size_t{ 1 } << (std::bit_width(space) - 1);

      const size_t cursor_align = cursor == 0 ? block : (size_t{ 1 } << std::countr_zero(cursor));
      block = block < cursor_align ? block : cursor_align;
      if (block < 32) {
        break;
      }

      freelist.push(free_list::bin_index(block), current_page->get_ptr_at(cursor));
      cursor += block;
    }
    current_page->cursor = page::kPageSize;
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
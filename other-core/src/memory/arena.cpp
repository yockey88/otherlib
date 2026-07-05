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
  namespace {

  }  // namespace

  arena::~arena() {
    storage.cleanup(page_allocation_cursor);
    page_allocation_cursor = 0;
  }

  void* arena::allocate(size_t size, size_t alignment) {
    PROFILE_SECTION("arena::allocate");
    OTHER_ASSERT(size > 0, "arena::allocate called with size 0.");
    OTHER_ASSERT(alignment <= page::kAlignment, "arena supports alignment <= {} (requested {}).", page::kAlignment, alignment);
    OTHER_ASSERT(size + sizeof(alloc_header) <= free_list::bin_block_size(free_list::kNumBins - 1), "Allocation size {} exceeds arena maximum.", size);

    auto& instance = instance_ref();
    std::lock_guard lock(instance.arena_mutex);

#ifdef USE_NEW_ALLOCATION
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
#else
    size_t padding = instance.get_allocation_padding(size, alignment);
    size_t actual_space_needed = size + padding;
    padding = instance.check_page_and_recalculate_padding(size, alignment, actual_space_needed);
    return instance.do_allocation(size, alignment, padding, actual_space_needed);
#endif
  }

  void arena::free(void* ptr, std::size_t size) {
    PROFILE_SECTION("arena::free");
#ifdef USE_NEW_ALLOCATION
    if (ptr != nullptr) {
      auto* header = static_cast<alloc_header*>(ptr) - 1;
      OTHER_ASSERT(header->magic == kAllocationTag, "arena::free called on a pointer the arena does not own.");
      OTHER_ASSERT(header->user_size == size, "arena::free size mismatch: allocated {}, freed {}.", header->user_size, size);
    }
    free(ptr);
#else
    if (ptr == nullptr) {
      return;
    }
    auto& instance = instance_ref();
    std::lock_guard lock_arena_mutex(instance.arena_mutex);
    instance.allocated_memory -= size;
    instance.live_allocations--;
    PROFILE_DEALLOCATION(ptr);
#endif
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
#ifdef USE_NEW_ALLOCATION
    instance.freelist.push(header->bin, header);
    instance.live_allocations--;
    instance.used_memory -= free_list::bin_block_size(header->bin);
#else
    //// be nice to have size info here but oh well
    instance.live_allocations--;
    PROFILE_DEALLOCATION(ptr);
#endif
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

  page* arena::request_memory_page() {
    return instance_ref().storage.create_page();
  }

  void arena::free_memory_page(page* region) {
    instance_ref().storage.free_page(region);
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
    /// caller holds arena_mutex. Decompose the tail of the outgoing page into pow2 blocks and assign them to a bin
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

}  // namespace other
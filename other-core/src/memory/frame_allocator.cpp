/**
 * \file core/frame_allocator.cpp
 **/
#include "memory/frame_allocator.hpp"

#include "core/logger.hpp"
#include "thread/thread_safety.hpp"

namespace other {

  frame_allocator::frame_allocator() {
    page = arena::request_memory_page();
    OTHER_ASSERT(page != nullptr, "Failed to allocate initial page for frame allocator.");
  }

  frame_allocator::~frame_allocator() {
    arena::free_memory_page(page);
  }

  void* frame_allocator::allocate(size_t size, size_t alignment) {
    ASSERT_MAIN_THREAD();
    OTHER_ASSERT(page != nullptr, "Frame allocator page is null.");
    const size_t unaligned = page->cursor;
    const size_t aligned_ptr = (unaligned + (alignment - 1)) & ~(alignment - 1);
    const size_t aligned = aligned_ptr;
    OTHER_ASSERT(aligned + size <= page::kPageSize, "frame_allocator overflow: {} + {} > {}.  Debug memory usage.", aligned, size, page::kPageSize);
    page->cursor = aligned + size;
    return page->get_ptr_at(aligned);
  }

  void frame_allocator::reset() {
    OTHER_ASSERT(page != nullptr, "Frame allocator page is null.");
    max_frame_usage = page->cursor > max_frame_usage ? page->cursor : max_frame_usage;
    page->cursor = 0;
  }

}  // namespace other
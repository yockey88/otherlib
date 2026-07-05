/**
 * \file core/frame_allocator.cpp
 **/
#include "memory/frame_allocator.hpp"

#include "core/logger.hpp"

namespace other {

  frame_allocator::frame_allocator() {
    page = arena::request_memory_page();
  }

  frame_allocator::~frame_allocator() {
    arena::free_memory_page(page);
  }

  void* frame_allocator::allocate(size_t size) {
    OTHER_ASSERT(page != nullptr, "Frame allocator page is null.");
    OTHER_ASSERT(page->cursor + size <= page::kPageSize, "Frame allocator out of memory for the current frame. Requested size: {}, Available size: {}.", size, page::kPageSize - page->cursor);

    void* ptr = page->get_ptr_at(page->cursor);
    page->cursor += size;
    frame_size += size;

    return ptr;
  }

  void frame_allocator::begin_frame() {
    OTHER_ASSERT(page != nullptr, "Frame allocator page is null.");
  }

  void frame_allocator::end_frame() {
    page->cursor = 0;
    frame_size = 0;
  }

}  // namespace other
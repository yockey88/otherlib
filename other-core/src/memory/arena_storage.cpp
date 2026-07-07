/**
 * \file core/arena_storage.cpp
 **/
#include "memory/arena_storage.hpp"

#include "core/logger.hpp"
#include "memory/frame_allocator.hpp"

namespace other {

  void arena_storage::cleanup(size_t page_count) {
    for (size_t i = 0; i < page_count; i++) {
      free_page(i);
    }
  }

  void arena_storage::finalize() {
    for (auto* frame : frame_allocators) {
      destroy_frame_allocator(frame);
    }
    frame_allocators.clear();
    for (auto* page : requested_pages) {
      destroy_page(page);
    }
    requested_pages.clear();
  }

  page* arena_storage::allocate_page(size_t idx) {
    OTHER_ASSERT(idx < kMaxPages, "Index out of bounds for page allocation.");
    PROFILE_SECTION("arena_storage::allocate_page");
    pages[idx] = new page();
    std::memset(pages[idx]->data(), 0, page::kPageSize);
    return pages[idx];
  }

  void arena_storage::free_page(size_t index) {
    OTHER_ASSERT(index < kMaxPages, "Index out of bounds for page allocation.");
    PROFILE_SECTION("arena_storage::free_page");
    delete pages[index];
    pages[index] = nullptr;
  }

  page* arena_storage::get_page(size_t idx) {
    if (idx >= kMaxPages || pages[idx] == nullptr) {
      return nullptr;
    }
    return pages[idx];
  }

  page* arena_storage::create_page() {
    page* p = new page();
    std::memset(p->data(), 0, page::kPageSize);
    requested_pages.push_back(p);
    return p;
  }

  void arena_storage::destroy_page(page* p) {
    if (p == nullptr) {
      return;
    }

    auto it = std::find(requested_pages.begin(), requested_pages.end(), p);
    if (it != requested_pages.end()) {
      delete p;
      requested_pages.erase(it);
    } else {
      CORE_LOG_ERROR("Attempted to destroy a page that was not allocated through arena_storage.");
    }
  }

  frame_allocator* arena_storage::create_frame_allocator() {
    auto* p = create_page();
    auto* frame = new frame_allocator(p);
    frame_allocators.push_back(frame);
    return frame;
  }

  void arena_storage::destroy_frame_allocator(frame_allocator* frame) {
    if (frame == nullptr) {
      return;
    }

    auto it = std::find(frame_allocators.begin(), frame_allocators.end(), frame);
    if (it != frame_allocators.end()) {
      destroy_page(frame->page);
      delete frame;
      frame_allocators.erase(it);
    } else {
      CORE_LOG_ERROR("Attempted to destroy a frame allocator that was not allocated through arena_storage.");
    }
  };

}  // namespace other
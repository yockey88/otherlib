/**
 * \file core/arena_storage.cpp
 **/
#include "memory/arena_storage.hpp"

#include "core/logger.hpp"

namespace other {

  void arena_storage::cleanup(size_t page_count) {
    for (size_t i = 0; i < page_count; i++) {
      free_page(i);
    }
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
    page* new_page = (page*)malloc(sizeof(page));
    active_pages.push_back(new_page);
    return new_page;
  }

  void arena_storage::free_page(page* p) {
    if (p == nullptr) {
      return;
    }

    auto it = std::find(active_pages.begin(), active_pages.end(), p);
    if (it != active_pages.end()) {
      active_pages.erase(it);
      free(p);
    } else {
      CORE_LOG_ERROR("Attempted to free a page that is not in the active pages list.");
    }
  }

}  // namespace other
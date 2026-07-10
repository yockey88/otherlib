/**
 * \file memory/arena_storage.hpp
 **/
#ifndef OTHER_CORE_MEMORY_ARENA_STORAGE_HPP
#define OTHER_CORE_MEMORY_ARENA_STORAGE_HPP

#include <vector>

#include "memory/page.hpp"

namespace other {

  class frame_allocator;

  struct arena_storage {
    static inline constexpr size_t kMaxPages = 64;
    static inline constexpr size_t kMaxMemoryAllowed = kMaxPages * page::kPageSize;

    void cleanup(size_t page_count);
    void finalize();

    page* allocate_page(size_t idx);
    void free_page(size_t index);

    page* get_page(size_t idx);

    page* create_page();
    void destroy_page(page* p);

    frame_allocator* create_frame_allocator();
    void destroy_frame_allocator(frame_allocator* frame);

   private:
#ifdef OTHER_TEST_ENVIRONMENT
    friend class arena_test;
#endif
    // core arena
    page* pages[kMaxPages];
    // memory pools and other that want to manage their own memory
    std::vector<page*> requested_pages;
    std::vector<frame_allocator*> frame_allocators;
  };

}  // namespace other

#endif  // OTHER_CORE_MEMORY_ARENA_STORAGE_HPP
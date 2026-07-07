/**
 * \file memory/arena_storage.hpp
 **/
#ifndef OTHER_CORE_MEMORY_ARENA_STORAGE_HPP
#define OTHER_CORE_MEMORY_ARENA_STORAGE_HPP

#include <cstdint>

#include "memory/page.hpp"

namespace other {

  struct arena_storage {
    static inline constexpr size_t kMaxPages = 64;
    static inline constexpr size_t kMaxMemoryAllowed = kMaxPages * page::kPageSize;

    void cleanup(size_t page_count);

    page* allocate_page(size_t idx);
    void free_page(size_t index);

    page* get_page(size_t idx);

    page* create_page();
    void destroy_page(page* p);

   private:
#ifdef OTHER_TEST_ENVIRONMENT
    friend class arena_test;
#endif
    // core arena
    page* pages[kMaxPages];
    // memory pools and other that want to manage their own memory
    std::vector<page*> requested_pages;
  };

}  // namespace other

#endif  // OTHER_CORE_MEMORY_ARENA_STORAGE_HPP
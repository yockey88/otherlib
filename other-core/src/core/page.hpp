/**
 * \file core/page.hpp
 **/
#ifndef OTHER_CORE_CORE_PAGE_HPP
#define OTHER_CORE_CORE_PAGE_HPP

#include <cstdint>
#include <type_traits>

namespace other {

  struct page {
    static inline constexpr size_t kPageSize = 4096 * 4096u;  // 16mb pages
    static inline constexpr size_t kAlignment = 16;

    size_t cursor = 0;
    /// \todo this alignment is not not correct for all types and causes
    ///       issue with things like asio::io_context (if we don't include it here it will be 1 byte aligned)
    alignas(kAlignment) uint8_t storage[kPageSize] = {};

    void* data() { return &storage[0]; }
    const void* data() const { return &storage[0]; }

    void* get_ptr_at(size_t offset);
  };
  static_assert(std::is_trivially_destructible_v<page>, "Page must be trivially destructible to avoid destructor calls on deallocation.");

}  // namespace other

#endif  // OTHER_CORE_CORE_PAGE_HPP
/**
 * \file memory/frame_allocator.hpp
 **/
#ifndef OTHER_CORE_MEMORY_FRAME_ALLOCATOR_HPP
#define OTHER_CORE_MEMORY_FRAME_ALLOCATOR_HPP

#include "memory/arena.hpp"

namespace other {

  struct frame_allocator {
    page* page = nullptr;
    size_t max_frame_usage = 0;
    uint32_t reference_count = 0;

    void* allocate(size_t size, size_t alignment = page::kAlignment);
    void reset();

    inline size_t get_max_frame_usage() const { return max_frame_usage; }
    frame_allocator(struct page* page);
    ~frame_allocator();
  };

}  // namespace other

#endif  // OTHER_CORE_MEMORY_FRAME_ALLOCATOR_HPP
/**
 * \file core/frame_allocator.hpp
 **/
#ifndef OTHER_CORE_CORE_FRAME_ALLOCATOR_HPP
#define OTHER_CORE_CORE_FRAME_ALLOCATOR_HPP

#include "core/arena.hpp"

namespace other {

  class frame_allocator {
   public:
    frame_allocator();
    ~frame_allocator();

    void* allocate(size_t size);

    void begin_frame();
    void end_frame();

   private:
    page* page = nullptr;
    size_t frame_size = 0;

    size_t current_offset = 0;
    size_t current_frame_index = 0;
  };

}  // namespace other

#endif  // OTHER_CORE_CORE_FRAME_ALLOCATOR_HPP
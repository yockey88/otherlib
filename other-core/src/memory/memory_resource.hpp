/**
 * \file memory/memory_resource.hpp
 **/
#ifndef OTHER_CORE_MEMORY_MEMORY_RESOURCE_HPP
#define OTHER_CORE_MEMORY_MEMORY_RESOURCE_HPP

#include "memory/arena.hpp"
#include "memory/frame_allocator.hpp"

namespace other {

  template <typename T>
  struct std_arena_allocator {
    using value_type = T;

    std_arena_allocator() = default;
    template <typename U>
    constexpr std_arena_allocator(const std_arena_allocator<U>&) noexcept {}

    T* allocate(size_t n) {
      return static_cast<T*>(arena::allocate(n * sizeof(T), alignof(T)));
    }

    void deallocate(T* ptr, size_t n) {
      arena::free(ptr, n * sizeof(T));
    }

    template <typename U>
    bool operator==(const std_arena_allocator<U>&) const { return true; }
  };

  template <typename T>
  struct std_frame_allocator {
    using value_type = T;

    std_frame_allocator() noexcept
        : frame(arena::create_frame_allocator()) {
      OTHER_ASSERT(frame != nullptr, "Failed to create frame allocator for std_frame_allocator.");
      ++frame->reference_count;
    }
    explicit std_frame_allocator(frame_allocator* frame) noexcept
        : frame(frame) {
      OTHER_ASSERT(frame != nullptr, "std_frame_allocator cannot be constructed with a null frame allocator.");
      ++frame->reference_count;
    }
    template <typename U>
    constexpr std_frame_allocator(const std_frame_allocator<U>& other) noexcept
        : frame(other.frame) {
      OTHER_ASSERT(frame != nullptr, "std_frame_allocator cannot be constructed with a null frame allocator.");
      ++frame->reference_count;
    }
    ~std_frame_allocator() {
      OTHER_ASSERT(frame != nullptr, "std_frame_allocator cannot have a null frame allocator.");
      --frame->reference_count;
      if (frame->reference_count == 0) {
        arena::destroy_frame_allocator(frame);
      }
    }

    using propagate_on_container_copy_assignment = std::true_type;
    using propagate_on_container_move_assignment = std::true_type;
    using propagate_on_container_swap = std::true_type;

    T* allocate(size_t n) { return static_cast<T*>(frame->allocate(n * sizeof(T), alignof(T))); }
    void deallocate(T* ptr, size_t n) { /* end of frame resets allocator*/ }

    template <typename U>
    bool operator==(const std_frame_allocator<U>&) const { return true; }

    frame_allocator* frame;
  };

}  // namespace other

#endif  // OTHER_CORE_MEMORY_MEMORY_RESOURCE_HPP
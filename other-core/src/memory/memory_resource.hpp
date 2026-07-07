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

    explicit std_frame_allocator(frame_allocator* frame) noexcept
        : frame(frame) {}
    template <typename U>
    constexpr std_frame_allocator(const std_frame_allocator<U>& other) noexcept
        : frame(other.frame) {}

    T* allocate(size_t n) {
      OTHER_ASSERT(frame != nullptr, "Frame allocator is null.");
      return static_cast<T*>(frame->allocate(n * sizeof(T), alignof(T)));
    }

    void deallocate(T* ptr, size_t n) { /* end of frame resets allocator*/ }

    template <typename U>
    bool operator==(const std_frame_allocator<U>&) const { return true; }

   private:
    frame_allocator* frame;
  };

}  // namespace other

#endif  // OTHER_CORE_MEMORY_MEMORY_RESOURCE_HPP
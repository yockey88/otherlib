/**
 * \file memory/memory_resource.hpp
 **/
#ifndef OTHER_CORE_MEMORY_MEMORY_RESOURCE_HPP
#define OTHER_CORE_MEMORY_MEMORY_RESOURCE_HPP

#include "memory/arena.hpp"

namespace other {

  template <typename T>
  struct std_arena_allocator {
    using value_type = T;

    std_arena_allocator() = default;
    template <typename U>
    constexpr std_arena_allocator(const std_arena_allocator<U>&) noexcept {}

    T* allocate(size_t n) {
      T* obj = static_cast<T*>(arena::allocate(n * sizeof(T), alignof(T)));
      OTHER_ASSERT(obj != nullptr, "Failed to allocate memory for {} objects.", n);
      return obj;
    }

    void deallocate(T* ptr, size_t n) {
      arena::free(ptr, n * sizeof(T));
    }

    template <typename U>
    bool operator==(const std_arena_allocator<U>&) const { return true; }
  };

}  // namespace other

#endif  // OTHER_CORE_MEMORY_MEMORY_RESOURCE_HPP
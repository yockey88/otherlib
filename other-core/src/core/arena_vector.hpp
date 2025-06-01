/**
 * @file core/arena_vector.hpp
 */
#ifndef OTHER_CORE_CORE_ARENA_VECTOR_HPP
#define OTHER_CORE_CORE_ARENA_VECTOR_HPP

#include <cstddef>

#include "core/arena_allocator.hpp"

namespace other {

  template <typename T>
  class arena_vector {
   public:
    arena_vector() = default;
    arena_vector(arena_vector&&) = default;
    arena_vector(const arena_vector&) = delete;
    arena_vector& operator=(arena_vector&&) = default;
    arena_vector& operator=(const arena_vector&) = delete;

    arena_vector(T* data, size_t size, size_t capacity)
        : data(data), size(size), capacity(capacity) {}
    arena_vector(size_t capacity)
        : data(nullptr), size(0), capacity(capacity) {}
    arena_vector(T* data, size_t size)
        : data(data), size(size), capacity(size) {}

    template <typename CT>
      requires std::is_same_v<typename CT::value_type, T>
    arena_vector(const CT& containter) {
      size = containter.size();
      capacity = containter.capacity();
    }

    ~arena_vector() = default;

   private:
    T* data = nullptr;
    size_t size = 0;
    size_t capacity = 0;

    static inline arena_allocator<T> allocator;
  };

}  // namespace other

#endif  // OTHER_CORE_CORE_ARENA_VECTOR_HPP
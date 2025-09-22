/**
 * \file data-structures/arena_array.hpp
 **/
#ifndef OTHER_CORE_DATA_STRUCTURES_ARENA_ARRAY_HPP
#define OTHER_CORE_DATA_STRUCTURES_ARENA_ARRAY_HPP

#include "core/arena_allocator.hpp"
#include "core/defines.hpp"
#include "core/logger.hpp"

namespace other {

  template <typename T, size_t N>
  class arena_array {
   public:
    arena_array() {
      allocate();
    }

    arena_array(arena_array&&) = default;
    arena_array& operator=(arena_array&&) = default;

    arena_array(const arena_array&) = delete;
    arena_array& operator=(const arena_array&) = delete;

    ~arena_array() {
      if (data != nullptr) {
        allocator.free(data);
        data = nullptr;
      }
    }

    T& operator[](natural_t i) {
      OTHER_ASSERT(i < size, "Index out of bounds in arena_array::operator[]");
      return data[i];
    }

    const natural_t capacity = N * sizeof(T);
    const natural_t size = N;

   private:
    T* data = nullptr;

    void allocate() {
      if (data == nullptr) {
        data = allocator.allocate_block(N);
        for (auto i = 0; i < size; i++) {
          new (&data[i]) T();
        }
      }
    }
    void free() {
      if (data != nullptr) {
        allocator.free_block(data, N);
        data = nullptr;
      }
    }

    static inline arena_allocator<T> allocator;
  };

}  // namespace other

#endif  // OTHER_CORE_DATA_STRUCTURES_ARENA_ARRAY_HPP
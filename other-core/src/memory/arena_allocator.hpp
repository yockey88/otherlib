/**
 * @file memory/arena_allocator.hpp
 */
#ifndef OTHER_CORE_MEMORY_ARENA_ALLOCATOR_HPP
#define OTHER_CORE_MEMORY_ARENA_ALLOCATOR_HPP

#include <cstring>
#include <new>
#include <type_traits>
#include <utility>

#include "memory/allocator.hpp"
#include "memory/arena.hpp"

namespace other {

  template <typename T>
  class arena_allocator : public allocator {
   public:
    using value_type = T;

    arena_allocator(arena* arena) noexcept
        : override_arena(arena) {}
    arena_allocator() = default;
    virtual ~arena_allocator() override = default;

    template <typename U>
    constexpr arena_allocator(const arena_allocator<U>&) noexcept {}

    void* allocate_bytes(size_t size) {
      if (override_arena != nullptr) {
        return override_arena->allocate(size);
      } else {
        return arena::allocate(size);
      }
    }
    void free_bytes(void* ptr, size_t size) {
      if (override_arena != nullptr) {
        override_arena->free(ptr, size);
      } else {
        arena::free(ptr, size);
      }
    }

    template <typename... Args>
    T* allocate(Args&&... args) {
      /// TODO: custom alignment
      void* memory = nullptr;
      if (override_arena != nullptr) {
        memory = override_arena->allocate(type_size, type_alignment);
      } else {
        memory = arena::allocate(type_size, type_alignment);
      }

      new (memory) T(std::forward<Args>(args)...);
      return std::launder(static_cast<T*>(memory));
    }

    T* allocate(const T& other)
      requires std::is_copy_constructible_v<T>
    {
      void* memory = nullptr;
      if (override_arena != nullptr) {
        memory = override_arena->allocate(type_size, type_alignment);
      } else {
        memory = arena::allocate(type_size, type_alignment);
      }

      new (memory) T(other);
      return std::launder(static_cast<T*>(memory));
    }

    T* allocate(T&& other)
      requires std::is_move_constructible_v<T>
    {
      void* memory = nullptr;
      if (override_arena != nullptr) {
        memory = override_arena->allocate(type_size, type_alignment);
      } else {
        memory = arena::allocate(type_size, type_alignment);
      }

      new (memory) T(std::move(other));
      return std::launder(static_cast<T*>(memory));
    }

    T* allocate_block(size_t size) {
      T* ptr = (T*)arena::allocate(size * type_size, type_alignment);
      for (size_t i = 0; i < size; i++) {
        new (&ptr[i]) T();
      }
      return ptr;
    }
    void free_block(T* ptr, size_t size) {
      for (size_t i = 0; i < size; i++) {
        ptr[i].~T();
      }

      if (override_arena != nullptr) {
        override_arena->free(ptr, size * type_size);
      } else {
        arena::free(ptr, size * type_size);
      }
    }

    void free(T* ptr) {
      if (ptr != nullptr) {
        ptr->~T();
      }

      if (override_arena != nullptr) {
        override_arena->free(ptr);
      } else {
        arena::free(ptr);
      }
    }

    static constexpr size_t type_size = sizeof(T);
    static constexpr size_t type_alignment = alignof(T);

   private:
    arena* override_arena = nullptr;
  };

  template <class T, class U>
  bool operator==(const arena_allocator<T>&, const arena_allocator<U>&) {
    return true;
  }

  template <class T, class U>
  bool operator!=(const arena_allocator<T>&, const arena_allocator<U>&) { return false; }

}  // namespace other

#endif  // OTHER_CORE_MEMORY_ARENA_ALLOCATOR_HPP

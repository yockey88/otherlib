/**
 * @file core/arena_allocator.hpp
 */
#ifndef OTHER_CORE_CORE_ARENA_ALLOCATOR_HPP
#define OTHER_CORE_CORE_ARENA_ALLOCATOR_HPP

#include <cstring>
#include <new>
#include <type_traits>
#include <utility>

#include "core/allocator.hpp"
#include "core/arena.hpp"

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

    template <typename... Args>
    T* allocate(Args&&... args) {
      /// TODO: custom alignment
      void* memory = nullptr;
      if (override_arena != nullptr) {
        memory = override_arena->allocate(type_size);
      } else {
        memory = arena::allocate(type_size);
      }

      new (memory) T(std::forward<Args>(args)...);
      return std::launder(static_cast<T*>(memory));
    }

    void* allocate_block(size_t size) {
      return arena::allocate(size);
    }

    void free(T* ptr) {
      if (ptr != nullptr) {
        std::destroy_at(ptr);
      }
      if (override_arena != nullptr) {
        override_arena->free(ptr, type_size);
      } else {
        arena::free(ptr, type_size);
      }
    }

    template <typename T2>
      requires std::is_base_of_v<T, T2>
    void free(T2* ptr) {
      free((T*)ptr);
    }

    static constexpr size_t type_size = sizeof(T);

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

#endif  // OTHER_CORE_CORE_ARENA_ALLOCATOR_HPP

/**
 * \file arena_allocator.hpp
 **/
#ifndef OTHER_CORE_ARENA_ALLOCATOR_HPP
#define OTHER_CORE_ARENA_ALLOCATOR_HPP

#include <cstring>
#include <new>
#include <utility>

#include "core/allocator.hpp"
#include "core/arena.hpp"

namespace other {

  template <typename T>
  class arena_allocator : public allocator {
   public:
    using value_type = T;

    arena_allocator() = default;
    virtual ~arena_allocator() override = default;

    template <typename U>
    constexpr arena_allocator(const arena_allocator<U>&) noexcept {}

    template <typename... Args>
    T* allocate(Args&&... args) {
      // PROFILE_SECTION("ArenaAllocator--Allocate");
      /// TODO: custom alignment
      void* memory = arena::allocate(type_size, alignof(T));
      if (memory == nullptr) {
        throw std::bad_alloc();
      }
      // PROFILE_ALLOCATION(memory, type_size);
      new (memory) T(std::forward<Args>(args)...);
      return std::launder(static_cast<T*>(memory));
    }

    void* allocate_block(size_t size) {
      return arena::allocate(size, alignof(T));
    }

    void free(T* ptr) {
      // PROFILE_SECTION("ArenaAllocator--Free");

      if (ptr != nullptr) {
        // PROFILE_DEALLOCATION(ptr);
        ptr->~T();
        void* memory = static_cast<void*>(ptr);
        std::memset(memory, 0, sizeof(T));
      }
      arena::free(ptr, type_size);
    }

    void free(void* ptr) {
      if (ptr != nullptr) {
        // PROFILE_DEALLOCATION(ptr);
        T* t_ptr = static_cast<T*>(ptr);
        t_ptr->~T();

        void* memory = static_cast<void*>(ptr);
        std::memset(memory, 0, sizeof(T));
      }
      arena::free(ptr, type_size);
    }

    /// cpp standard allocator interface
    [[nodiscard]] T* allocate(size_t size) {
      return static_cast<T*>(allocate_block(size));
    }

    void deallocate(void* ptr, size_t) noexcept {
      free(static_cast<T*>(ptr));
    }

    static constexpr size_t type_size = sizeof(T);
  };

  template <class T, class U>
  bool operator==(const arena_allocator<T>&, const arena_allocator<U>&) {
    return true;
  }

  template <class T, class U>
  bool operator!=(const arena_allocator<T>&, const arena_allocator<U>&) { return false; }

}  // namespace other

#endif  // OTHER_CORE_ARENA_ALLOCATOR_HPP

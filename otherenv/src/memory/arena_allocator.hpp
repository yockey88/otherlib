/**
 * \file memory/arena_allocator.hpp
 **/
#ifndef OTHERENV_MEMORY_ARENA_ALLOCATOR_HPP
#define OTHERENV_MEMORY_ARENA_ALLOCATOR_HPP

#include <cstring>
#include <new>
#include <utility>

#include "memory/allocator.hpp"
#include "memory/arena.hpp"

// #include "kernel/profiler.hpp"

namespace other {

  template <typename T>
  class ArenaAllocator : public Allocator {
   public:
    using value_type = T;

    ArenaAllocator() = default;
    virtual ~ArenaAllocator() override = default;

    template <typename U>
    constexpr ArenaAllocator(const ArenaAllocator<U>&) noexcept {}

    template <typename... Args>
    T* Allocate(Args&&... args) {
      // PROFILE_SECTION("ArenaAllocator--Allocate");
      /// TODO: custom alignment
      void* memory = Arena::Allocate(type_size);
      if (memory == nullptr) {
        throw std::bad_alloc();
      }
      // PROFILE_ALLOCATION(memory, type_size);
      new (memory) T(std::forward<Args>(args)...);
      return std::launder(static_cast<T*>(memory));
    }

    void* AllocateBlock(size_t size) {
      return Arena::Allocate(size);
    }

    void Free(T* ptr) {
      // PROFILE_SECTION("ArenaAllocator--Free");

      if (ptr != nullptr) {
        // PROFILE_DEALLOCATION(ptr);
        ptr->~T();
        void* memory = static_cast<void*>(ptr);
        std::memset(memory, 0, sizeof(T));
      }
      Arena::Free(ptr, type_size);
    }

    void Free(void* ptr) {
      if (ptr != nullptr) {
        // PROFILE_DEALLOCATION(ptr);
        T* t_ptr = static_cast<T*>(ptr);
        t_ptr->~T();

        void* memory = static_cast<void*>(ptr);
        std::memset(memory, 0, sizeof(T));
      }
      Arena::Free(ptr, type_size);
    }

    /// cpp standard allocator interface
    [[nodiscard]] T* allocate(size_t size) {
      return static_cast<T*>(AllocateBlock(size));
    }

    void deallocate(void* ptr, size_t) noexcept {
      Free(static_cast<T*>(ptr));
    }

    static constexpr size_t type_size = sizeof(T);
  };

  template <class T, class U>
  bool operator==(const ArenaAllocator<T>&, const ArenaAllocator<U>&) {
    return true;
  }

  template <class T, class U>
  bool operator!=(const ArenaAllocator<T>&, const ArenaAllocator<U>&) { return false; }

}  // namespace other

#endif  // !OTHERENV_MEMORY_ARENA_ALLOCATOR_HPP

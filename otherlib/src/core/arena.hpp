/**
 * \file core/arena.hpp
 **/
#ifndef OTHER_CORE_MEMORY_ARENA_HPP
#define OTHER_CORE_MEMORY_ARENA_HPP

#include <cstdint>
#ifndef OTHERENV_WINDOWS
  #include <cstddef>
#endif

#include "core/subsystem.hpp"

namespace other {

  // class Registers;

  class arena : public subsystem<arena> {
   public:
    static inline constexpr size_t kPageSize = 64 * 4096u * 4096u;
    /// TODO: this feels wrong, this is correct for the GPU but seems incorrect if
    ///         aiming to be as cross platform as possible (research: confirm all
    ///         GPUs read mem in 16 byte chunks)
    static inline constexpr size_t kAlignment = 16;
    static inline constexpr size_t kMaxPages = 16;
    static inline constexpr size_t kMaxMemoryAllowed =
      arena::kMaxPages * arena::kPageSize;

    template <size_t size, size_t alignment = kAlignment>
    struct page {
      size_t cursor = 0;
      std::aligned_storage_t<size, alignment> storage;
    };

    arena() = default;
    ~arena();

    static void* allocate(size_t size, size_t alignment);
    static void free(void* ptr, size_t size);

   private:
    // friend class ModuleRegistry;

    // static void set_arena_instance(arena* arena);

    // arena(arena&&) = delete;
    // arena(const arena&) = delete;
    // arena& operator=(arena&&) = delete;
    // arena& operator=(const arena&) = delete;

    // static arena* instance;

    // Registers* registers = nullptr;

    size_t page_allocation_cursor = 0;
    size_t page_cursor = 0;
    size_t total_allocations = 0;
    size_t allocated_memory = 0;

    uint8_t* pages[kMaxPages];

    void allocate_page();

    // #ifdef OTHERENV_MEMORY_DEBUG
    //     static void ReportAllocation(void* ptr, std::size_t size);
    // #endif
  };

  template <>
  struct subsystem_description<arena> {
    static constexpr size_t size = sizeof(arena);
    static constexpr size_t alignment = alignof(arena);
    static inline subsystem_storage_t<arena> storage;

    static arena* ptr() {
      return std::launder(reinterpret_cast<arena*>(&storage));
    }

    static void* address() {
      return reinterpret_cast<void*>(&storage);
    }
  };

}  // namespace other

#endif  // OTHER_CORE_MEMORY_ARENA_HPP

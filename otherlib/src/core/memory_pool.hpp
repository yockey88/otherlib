/**
 * \file core/memory_pool.hpp
 **/
#ifndef OTHER_CORE_MEMORY_POOL_HPP
#define OTHER_CORE_MEMORY_POOL_HPP

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <new>
#include <span>

// #include "profiler.hpp"
#include "core/defines.hpp"
#include "core/ref_counted.hpp"

namespace other {

  template <typename T>
    requires std::default_initializable<T>
  class memory_pool : public ref_counted {
   public:
    memory_pool(size_t max_objects)
        : capacity(max_objects) {
      allocate_block();
    }

    template <typename... Args>
      requires std::constructible_from<T, Args...>
    memory_pool(size_t max_objects, Args&&... init_args)
        : capacity(max_objects) {
      allocate_block();

      /// this constructor constructs all objcects with init_args
      for (size_t i = 0; i < max_objects; i++) {
        new (pool->memory + (i * sizeof(T))) T(std::forward<Args>(init_args)...);
      }
    }

    ~memory_pool() {
      free_block();
    }

    /// \todo implement move semantics
    // memory_pool(memory_pool&&) = delete;
    // memory_pool& operator=(memory_pool&&) = delete;

    // no copy constructors, force users to move the memory pool
    memory_pool(const memory_pool&) = delete;
    memory_pool& operator=(const memory_pool&) = delete;

    T& operator[](size_t idx) {
      // OE_ASSERT(idx < max_objects, "Index out of bounds");
      /// safe to use temp object because span is a view and does not own the memory
      return objects()[idx];
    }

    const T& operator[](size_t idx) const {
      // OE_ASSERT(idx < max_objects, "Index out of bounds");
      /// safe to use temp object because span is a view and does not own the memory
      return objects()[idx];
    }

    std::pair<T&, size_t> emplace() {
      // size_t idx = find_first_free_object();
      throw std::runtime_error("Not implemented");
    }

    T* at(size_t idx) { return &objects()[idx]; }
    const T* at(size_t idx) const { return &objects()[idx]; }

    const size_t max_objects() const { return capacity; }

    std::span<T> objects() { return std::span<T>(std::launder(reinterpret_cast<T*>(pool->memory)), num_objects); }
    const std::span<const T> objects() const { return std::span<const T>(std::launder(reinterpret_cast<T*>(pool->memory)), num_objects); }

   private:
    /// \todo fix the storage to be usable with types of possibly different alignments/sizes so we can store derived types etc...
    struct storage {
      storage(size_t max_objects)
          : capacity(max_objects), type_size(sizeof(T)), alignment(alignof(T)) {
        memory = new (std::align_val_t{ alignment }) uint8_t[max_objects * type_size];
        if (memory == nullptr) {
          throw std::bad_alloc();
        }
      }
      ~storage() { delete[] memory; }

      size_t capacity = 0;
      size_t type_size = 0;
      size_t alignment = 0;

      uint8_t* memory = nullptr;
    };

    scope<storage> pool = nullptr;

    size_t num_objects = 0;
    size_t capacity = 0;

    struct obj_flags {
      bool is_free = true;
    };
    std::vector<obj_flags> object_flags;

    void allocate_block() {
      free_block();
      pool = make_scope<storage>(capacity);

      object_flags = std::vector<obj_flags>(capacity);
      for (size_t i = 0; i < capacity; i++) {
        object_flags[i].is_free = true;
      }
      num_objects = 0;
    }

    void free_block() {
      pool = nullptr;
      object_flags.clear();
    }

    bool try_to_allocate(size_t idx) {
      if (idx >= capacity) {
        return false;
      }

      if (!object_flags[idx].is_free) {
        return false;
      } else {
        num_objects++;
      }
    }
  };

}  // namespace other

#endif  // OTHER_CORE_MEMORY_POOL_HPP
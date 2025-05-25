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
      for (size_t i = 0; i < max_objects; i++) {
        new (pool->memory + (i * sizeof(T))) T(std::forward<Args>(init_args)...);
      }
    }

    ~memory_pool() {
      free_block();
    }

    memory_pool(memory_pool&&) = delete;
    memory_pool(const memory_pool&) = delete;
    memory_pool& operator=(memory_pool&&) = delete;
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

    T* at(size_t idx) { return &objects()[idx]; }

    const T* at(size_t idx) const { return &objects()[idx]; }
    const size_t max_objects() const { return capacity; }

    std::span<T> objects() { return std::span<T>(std::launder(reinterpret_cast<T*>(pool->memory)), num_objects); }
    const std::span<const T> objects() const { return std::span<const T>(std::launder(reinterpret_cast<T*>(pool->memory)), num_objects); }

   private:
    /// storage for the memory pool
    template <typename U = T>
      requires std::default_initializable<U>
    struct storage {
      storage(size_t max_objects)
          : capacity(max_objects), type_size(sizeof(U)), alignment(alignof(U)) {
        memory = new (std::align_val_t{ alignment }) uint8_t[max_objects * type_size];
        if (memory == nullptr) {
          throw std::bad_alloc();
        }

        for (size_t i = 0; i < max_objects; i++) {
          new (memory + (i * this->type_size)) T();
        }
      }
      ~storage() { delete[] memory; }

      size_t capacity = 0;
      size_t type_size = 0;
      size_t alignment = 0;

      uint8_t* memory = nullptr;
    };

    scope<storage<T>> pool = nullptr;

    size_t num_objects = 0;
    size_t capacity = 0;

    struct obj_flags {
      bool is_free = true;
    };
    std::vector<obj_flags> object_flags;

    void allocate_block() {
      free_block();
      pool = make_scope<storage<T>>(capacity);

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
  };

}  // namespace other

#endif  // OTHER_CORE_MEMORY_POOL_HPP
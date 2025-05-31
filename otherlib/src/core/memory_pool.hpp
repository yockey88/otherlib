/**
 * \file core/memory_pool.hpp
 **/
#ifndef OTHER_CORE_MEMORY_POOL_HPP
#define OTHER_CORE_MEMORY_POOL_HPP

#include <concepts>
#include <cstddef>
#include <cstring>
#include <new>
#include <span>

#include "core/logger.hpp"
#include "core/ref_counted.hpp"

namespace other {

  template <typename T, size_t Max = 1024>
    requires std::default_initializable<T>
  class memory_pool : public ref_counted {
   public:
    using storage_type = std::aligned_storage_t<sizeof(T) * Max, alignof(T)>;

    memory_pool() {
    }

    ~memory_pool() {
      free_block();
    }

    memory_pool(memory_pool&& other) {
      free_block();
      allocate_block();

      pool = std::move(other.pool);
      num_objects = other.num_objects;
      object_flags = std::move(other.object_flags);

      other.num_objects = 0;
      other.capacity = 0;
      other.full = false;
      other.object_flags.clear();
      other.pool = storage_type();
    }
    memory_pool& operator=(memory_pool&& other) {
      if (this != &other) {
        free_block();
        allocate_block();

        pool = std::move(other.pool);
        num_objects = other.num_objects;
        object_flags = std::move(other.object_flags);

        other.num_objects = 0;
        other.capacity = 0;
        other.full = false;
        other.object_flags.clear();
        other.pool = storage_type();
      }
      return *this;
    }

    // no copy constructors, force users to move the memory pool
    memory_pool(const memory_pool&) = delete;
    memory_pool& operator=(const memory_pool&) = delete;

    T* at(size_t idx) { return &objects()[idx]; }
    const T* at(size_t idx) const { return &objects()[idx]; }

    T& operator[](size_t idx) {
      OTHER_ASSERT(idx < Max, "Index out of bounds");
      return objects()[idx];
    }

    const T& operator[](size_t idx) const {
      OTHER_ASSERT(idx < Max, "Index out of bounds");
      return objects()[idx];
    }

    std::pair<T&, size_t> emplace() {
      OTHER_ASSERT(!full, "Memory pool is full, cannot allocate more objects.");
      /// save the index before incrementing num_objects
      size_t idx = num_objects++;
      if (idx >= max_objects()) {
        OTHER_ASSERT(false, "Memory pool is full, cannot allocate more objects.");
        return { create_object(idx), idx };
      }
    }

    const size_t max_objects() const { return Max; }

    std::span<T> objects() { return std::span<T>(get_array(), num_objects); }
    const std::span<const T> objects() const { return std::span<const T>(get_array(), num_objects); }

   private:
    bool full = false;

    storage_type pool;

    size_t num_objects = 0;

    struct obj_flags {
      bool is_free = true;
    };
    std::vector<obj_flags> object_flags;

    T& create_object(size_t idx) {
      OTHER_ASSERT(idx < max_objects(), "Index out of bounds");
      OTHER_ASSERT(object_flags[idx].is_free, "Object at index {} is already allocated", idx);

      object_flags[idx].is_free = false;

      return *new (get_memory_raw_at(idx)) T();
    }

    T* get_array() { return std::launder(reinterpret_cast<T*>(get_memory_raw())); }
    const T* get_array() const { return std::launder(reinterpret_cast<const T*>(get_memory_raw())); }

    T* get_array_at(size_t idx) {
      OTHER_ASSERT(idx < max_objects(), "Index out of bounds");
      return std::launder(reinterpret_cast<T*>(get_memory_raw_at(idx)));
    }
    const T* get_array_at(size_t idx) const {
      OTHER_ASSERT(idx < max_objects(), "Index out of bounds");
      return std::launder(reinterpret_cast<const T*>(get_memory_raw_at(idx)));
    }

    void* get_memory_raw() { return reinterpret_cast<void*>(&pool); }
    const void* get_memory_raw() const { return reinterpret_cast<const void*>(&pool); }

    void* get_memory_raw_at(size_t idx) {
      OTHER_ASSERT(idx < max_objects(), "Index out of bounds");
      return get_memory_raw() + (idx * sizeof(T));
    }
    const void* get_memory_raw_at(size_t idx) const {
      OTHER_ASSERT(idx < max_objects(), "Index out of bounds");
      return get_memory_raw() + (idx * sizeof(T));
    }

    void allocate_block() {
      std::memset(&pool, 0, sizeof(storage_type));

      object_flags = std::vector<obj_flags>(max_objects());
      for (size_t i = 0; i < max_objects(); i++) {
        object_flags[i].is_free = true;
      }
      num_objects = 0;
    }

    void free_block() {
      std::memset(&pool, 0, sizeof(storage_type));
      object_flags.clear();
    }

    bool try_to_allocate(size_t idx) {
      if (idx >= max_objects()) {
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
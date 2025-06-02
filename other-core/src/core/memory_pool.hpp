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
#include <type_traits>

#include "core/logger.hpp"
#include "core/ref_counted.hpp"

namespace other {

  template <typename T, size_t Max = 1024>
    requires requires() { T{}; }
  class memory_pool : public ref_counted {
   public:
    static_assert(Max > 0, "Memory pool size must be greater than 0");

    static constexpr inline size_t kMaxObjects = Max;
    static constexpr inline size_t kMaxSize = sizeof(T) * Max;
    using storage_type = std::aligned_storage_t<sizeof(T) * Max, alignof(T)>;

    memory_pool() {}
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

    void clear() {
      free_block();
    }

    void free(size_t idx) {
      OTHER_ASSERT(idx < max_objects(), "Index out of bounds");
      if (idx >= max_objects() || !object_flags[idx].is_free) {
        return;
      }

      destory_object(idx);
    }

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
        /// \todo: defragment memory to see if there are any free slots and move all objects to the front,
        /// for now we will just find the first free slot
        for (size_t i = 0; i < max_objects(); ++i) {
          if (object_flags[i].is_free) {
            idx = i;
            break;
          }
        }

        if (idx >= max_objects()) {
          OTHER_ASSERT(false, "Memory pool is full, cannot allocate more objects.");
        }
      }
      return { create_object(idx), idx };
    }

    const size_t max_objects() const { return Max; }

    std::span<T> objects() { return std::span<T>(get_array(), num_objects); }
    const std::span<const T> objects() const { return std::span<const T>(get_array(), num_objects); }

   private:
    /// consider adding later if scene splits onto it's own simulation thread
    // std::mutex pool_mutex;
    bool full = false;

    storage_type pool;

    size_t num_objects = 0;

    struct obj_flags {
      bool is_free = true;
    };
    std::array<obj_flags, Max> object_flags;

    T& create_object(size_t idx) {
      // std::lock_guard lock(pool_mutex);
      OTHER_ASSERT(idx < max_objects(), "Index out of bounds");
      OTHER_ASSERT(object_flags[idx].is_free, "Object at index {} is already allocated", idx);

      object_flags[idx].is_free = false;

      T* obj = new (get_memory_raw_at(idx)) T();
      OTHER_ASSERT(obj != nullptr, "Failed to allocate memory for object at index {}", idx);
      return *obj;
    }

    void destory_object(size_t idx) {
      // std::lock_guard lock(pool_mutex);
      OTHER_ASSERT(idx < max_objects(), "Index out of bounds");
      OTHER_ASSERT(!object_flags[idx].is_free, "Object at index {} is already free", idx);

      T* obj = get_array_at(idx);
      OTHER_ASSERT(obj != nullptr, "Object at index {} is null", idx);
      if (obj) {
        if constexpr (std::is_nothrow_destructible_v<T>) {
          obj->~T();
        }

        /// clear the memory at the index
        std::memset(get_memory_raw_at(idx), 0, sizeof(T));
      }
      if (num_objects == 0) {
        full = false;
      }

      object_flags[idx].is_free = true;
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
      return reinterpret_cast<void*>((&pool) + (idx * sizeof(T)));
    }
    const void* get_memory_raw_at(size_t idx) const {
      OTHER_ASSERT(idx < max_objects(), "Index out of bounds");
      return reinterpret_cast<const void*>((&pool) + (idx * sizeof(T)));
    }

    void allocate_block() {
      // std::lock_guard lock(pool_mutex);

      std::memset(&pool, 0, sizeof(storage_type));
      object_flags = std::vector<obj_flags>(max_objects());
      for (size_t i = 0; i < max_objects(); i++) {
        object_flags[i].is_free = true;
      }
      num_objects = 0;
    }

    void free_block() {
      // std::lock_guard lock(pool_mutex);

      for (size_t i = 0; i < num_objects; ++i) {
        if (!object_flags[i].is_free) {
          destory_object(i);
        }
      }

      std::memset(&pool, 0, sizeof(storage_type));
      std::ranges::fill(object_flags, obj_flags{ true });
      num_objects = 0;
      full = false;
    }
  };

}  // namespace other

#endif  // OTHER_CORE_MEMORY_POOL_HPP
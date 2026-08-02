/**
 * \file core/memory_pool.hpp
 **/
#ifndef OTHER_CORE_MEMORY_POOL_HPP
#define OTHER_CORE_MEMORY_POOL_HPP

#include <cstddef>
#include <cstring>
#include <new>
#include <span>
#include <type_traits>

#include "core/logger.hpp"
#include "core/profiler.hpp"
#include "core/ref_counted.hpp"

namespace other {

  template <typename T, size_t Max = 1024>
    requires requires() { T{}; }
  class memory_pool : public ref_counted {
   public:
    static_assert(Max > 0, "Memory pool size must be greater than 0");
    static constexpr inline size_t kMaxObjects = Max;
    static constexpr inline size_t kMaxSize = sizeof(T) * Max;

#pragma pack(push, 1)
    struct storage_type {
      static constexpr size_t storage_size = kMaxSize;
      alignas(alignof(T)) uint8_t data[kMaxSize] = {};
    };
#pragma pack(pop)

    memory_pool(bool allocate_all = false) {
      if (allocate_all) {
        allocate_block();
      }
    }
    ~memory_pool() {
      free_block();
    }

    memory_pool(memory_pool&& other) {
      *this = std::move(other);
    }
    memory_pool& operator=(memory_pool&& other) {
      if (this != &other) {
        free_block();
        allocate_block();

        pool = std::move(other.pool);
        num_objects = other.num_objects;
        num_live = other.num_live;
        is_full = other.is_full;
        object_flags = std::move(other.object_flags);

        other.num_objects = 0;
        other.num_live = 0;
        other.is_full = false;
        other.object_flags = {};
        other.pool = {};
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
      PROFILE_SECTION("memory_pool::free");
      OTHER_ASSERT(idx < max_objects(), "Index out of bounds");
      if (idx >= max_objects() || object_flags[idx].is_free) {
        return;
      }

      destroy_object(idx);
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
      PROFILE_SECTION("memory_pool::emplace");

      OTHER_ASSERT(!is_full, "Memory pool is full, cannot allocate more objects.");
      size_t idx = acquire_slot();
      num_live++;
      if (num_live >= max_objects()) {
        is_full = true;
      }
      return { create_object(idx), idx };
    }

    std::pair<T&, size_t> emplace(T&& value) {
      PROFILE_SECTION("memory_pool::emplace");

      OTHER_ASSERT(!is_full, "Memory pool is full, cannot allocate more objects.");
      size_t idx = acquire_slot();
      num_live++;
      if (num_live >= max_objects()) {
        is_full = true;
      }
      return { create_object(idx, std::move(value)), idx };
    }

    const size_t size() const { return num_live; }
    const size_t free_objects() const { return max_objects() - num_live; }
    const size_t max_objects() const { return Max; }
    const size_t object_count() const { return num_live; }

    const bool full() const { return is_full; }
    const bool empty() const { return num_live == 0; }

    std::span<T> objects() { return std::span<T>(get_array(), Max); }
    const std::span<const T> objects() const { return std::span<const T>(get_array(), Max); }

    /// iterators
    auto begin() { return objects().begin(); }
    auto end() { return objects().end(); }
    auto begin() const { return objects().begin(); }
    auto end() const { return objects().end(); }

   private:
    /// consider adding later if scene splits onto it's own simulation thread
    std::mutex pool_mutex;
    bool is_full = false;

    storage_type pool;

    /// high-water allocation cursor
    /// slots below it are handed out first
    /// freed slots below it are only reused once it reaches Max
    /// never decremented freed slots are found by scanning object_flags
    size_t num_objects = 0;

    /// number of currently-allocated objects; drives is_full/size()
    size_t num_live = 0;

    /// next slot for a new object: the cursor while it lasts, then the first free slot
    size_t acquire_slot() {
      if (num_objects < max_objects()) {
        return num_objects++;
      }

      /// \todo: defragment memory to see if there are any free slots and move all objects to the front,
      /// for now we will just find the first free slot
      for (size_t i = 0; i < max_objects(); ++i) {
        if (object_flags[i].is_free) {
          return i;
        }
      }

      OTHER_ASSERT(false, "Memory pool is full, cannot allocate more objects.");
      return max_objects();
    }

    struct obj_flags {
      bool is_free = true;
    };
    std::array<obj_flags, Max> object_flags;

    T& create_object(size_t idx) {
      PROFILE_SECTION("memory_pool::create_object");

      OTHER_ASSERT(idx < max_objects(), "Index out of bounds");
      OTHER_ASSERT(object_flags[idx].is_free, "Object at index {} is already allocated", idx);

      object_flags[idx].is_free = false;

      T* obj = new (get_memory_raw_at(idx)) T();
      OTHER_ASSERT(obj != nullptr, "Failed to allocate memory for object at index {}", idx);
      return *obj;
    }

    T& create_object(size_t idx, T&& value) {
      PROFILE_SECTION("memory_pool::create_object");

      OTHER_ASSERT(idx < max_objects(), "Index out of bounds");
      OTHER_ASSERT(object_flags[idx].is_free, "Object at index {} is already allocated", idx);

      object_flags[idx].is_free = false;

      T* obj = new (get_memory_raw_at(idx)) T(std::move(value));
      OTHER_ASSERT(obj != nullptr, "Failed to allocate memory for object at index {}", idx);
      return *obj;
    }

    void destroy_object(size_t idx) {
      PROFILE_SECTION("memory_pool::destroy_object");

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
      num_live--;
      is_full = false;

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
      return reinterpret_cast<void*>((pool.data) + (idx * sizeof(T)));
    }
    const void* get_memory_raw_at(size_t idx) const {
      OTHER_ASSERT(idx < max_objects(), "Index out of bounds");
      return reinterpret_cast<const void*>((pool.data) + (idx * sizeof(T)));
    }

    void allocate_block() {
      PROFILE_SECTION("memory_pool::allocate_block");
      {
        std::lock_guard lock(pool_mutex);
        std::memset(pool.data, 0, storage_type::storage_size);

        object_flags = {};
        for (size_t i = 0; i < max_objects(); i++) {
          object_flags[i].is_free = true;
        }
        num_objects = 0;
        num_live = 0;
        is_full = false;
      }
    }

    void free_block() {
      PROFILE_SECTION("memory_pool::free_block");
      {
        std::lock_guard lock(pool_mutex);

        for (size_t i = 0; i < num_objects; ++i) {
          if (!object_flags[i].is_free) {
            destroy_object(i);
          }
        }

        std::memset(pool.data, 0, storage_type::storage_size);
        std::ranges::fill(object_flags, obj_flags{ true });
        num_objects = 0;
        num_live = 0;
        is_full = false;
      }
    }
  };

}  // namespace other

#endif  // OTHER_CORE_MEMORY_POOL_HPP
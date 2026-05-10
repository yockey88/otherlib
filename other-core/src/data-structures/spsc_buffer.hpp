/**
 * \file data-structures/spsc_buffer.hpp
 **/
#ifndef OTHER_CORE_DATA_STRUCTURES_SPSC_BUFFER_HPP
#define OTHER_CORE_DATA_STRUCTURES_SPSC_BUFFER_HPP

#include <atomic>
#include <type_traits>

#include "core/defines.hpp"

namespace other {

  template <typename T, size_t Cap>
    requires std::is_move_constructible_v<T>
  class spsc_buffer {
   public:
    static_assert(Cap > 0, "Capacity must be greater than 0.");
    static_assert(Cap % 2 == 0, "Capacity must be even to avoid ambiguity between full and empty states.");

    void push(const T& item) {
      push(T(item));
    }

    /// \note will overwrite if full
    void push(T&& item) {
      natural_t write_index = write_idx.load(std::memory_order_relaxed);
      natural_t next_write_index = write_index + 1;

      if (size() == Cap) {
        // Buffer is full, advance the read index to overwrite the oldest item
        natural_t next_read_index = read_idx.load(std::memory_order_relaxed) + 1;
        read_idx.store(next_read_index, std::memory_order_release);
      }

      void* slot = get_memory_for_slot(write_index);
      new (slot) T(std::move(item));

      write_idx.store(next_write_index, std::memory_order_release);
    }

    opt<T> pop() {
      if (empty()) {
        return std::nullopt;
      }

      natural_t read_index = read_idx.load(std::memory_order_relaxed);
      natural_t next_read_index = read_index + 1;

      void* slot = get_memory_for_slot(read_index);
      T* item_ptr = static_cast<T*>(slot);
      T item = std::move(*item_ptr);
      std::destroy_at(item_ptr);

      read_idx.store(next_read_index, std::memory_order_release);

      return item;
    }

    inline natural_t capacity() const { return Cap; }
    inline natural_t size() const {
      natural_t write_index = write_idx.load(std::memory_order_acquire);
      natural_t read_index = read_idx.load(std::memory_order_acquire);
      return write_index - read_index;
    }
    inline bool empty() const {
      natural_t write_index = write_idx.load(std::memory_order_acquire);
      natural_t read_index = read_idx.load(std::memory_order_acquire);
      return write_index == read_index;
    }

   private:
    alignas(kCacheLineSize) std::atomic<natural_t> read_idx{ 0 };
    alignas(kCacheLineSize) std::atomic<natural_t> write_idx{ 0 };

    alignas(kCacheLineSize) natural_t read_idx_cache = 0;
    alignas(kCacheLineSize) natural_t write_idx_cache = 0;

    alignas(kCacheLineSize) std::array<uint8_t, sizeof(T) * Cap> buffer;

    inline auto mask(natural_t index) const {
      return index & (Cap - 1);
    }

    void* get_memory_for_slot(natural_t index) {
      return static_cast<void*>(buffer.data() + (index * sizeof(T)));
    }
  };

}  // namespace other

#endif  // OTHER_CORE_DATA_STRUCTURES_SPSC_BUFFER_HPP
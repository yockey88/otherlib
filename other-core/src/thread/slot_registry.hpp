/**
 * \file thread/slot_registry.hpp
 **/
#ifndef OTHER_CORE_THREAD_SLOT_REGISTRY_HPP
#define OTHER_CORE_THREAD_SLOT_REGISTRY_HPP

#include <array>
#include <atomic>
#include <cstddef>

namespace other {

  /// fixed-capacity pointer registry: one mutating thread, any number of reader threads.
  ///  readers iterate wait-free (no locks anywhere); writers publish with a store and
  ///  retire with a tombstone. contract the owner must uphold:
  ///   - insert/erase/reset/contains run on a single thread at a time (the writer)
  ///   - erasing a slot does NOT make the object safe to destroy: a reader may still be
  ///     using the pointer it loaded before the tombstone. destroy only after the reader
  ///     has provably moved past (see network_thread::reclamation_epoch) or exited
  template <typename T, size_t Capacity>
  class slot_registry {
   public:
    /// writer: publish; false when full or already present
    bool insert(T* entry) {
      if (entry == nullptr || contains(entry)) {
        return false;
      }

      const size_t n = published.load(std::memory_order_relaxed);
      for (size_t i = 0; i < n; ++i) {
        if (slots[i].load(std::memory_order_relaxed) == nullptr) {
          slots[i].store(entry, std::memory_order_release);
          return true;
        }
      }

      if (n >= Capacity) {
        return false;
      }
      /// slot before count: a reader scanning [0, published) never sees an unwritten slot
      slots[n].store(entry, std::memory_order_relaxed);
      published.store(n + 1, std::memory_order_release);
      return true;
    }

    /// writer: tombstone; false when absent. seq_cst pairs with the reclamation-epoch
    ///  sampling that gates destruction of the pointee
    bool erase(T* entry) {
      const size_t n = published.load(std::memory_order_relaxed);
      for (size_t i = 0; i < n; ++i) {
        if (slots[i].load(std::memory_order_relaxed) == entry) {
          slots[i].store(nullptr, std::memory_order_seq_cst);
          return true;
        }
      }
      return false;
    }

    /// writer: tombstone everything (fresh lifecycle)
    void reset() {
      const size_t n = published.load(std::memory_order_relaxed);
      for (size_t i = 0; i < n; ++i) {
        slots[i].store(nullptr, std::memory_order_seq_cst);
      }
    }

    /// writer-accurate; racing readers get a best-effort count
    bool contains(const T* entry) const {
      const size_t n = published.load(std::memory_order_acquire);
      for (size_t i = 0; i < n; ++i) {
        if (slots[i].load(std::memory_order_relaxed) == entry) {
          return true;
        }
      }
      return false;
    }

    size_t count() const {
      size_t occupied = 0;
      const size_t n = published.load(std::memory_order_acquire);
      for (size_t i = 0; i < n; ++i) {
        if (slots[i].load(std::memory_order_relaxed) != nullptr) {
          ++occupied;
        }
      }
      return occupied;
    }

    /// any thread, wait-free: fn(T&) over every live entry
    template <typename Fn>
    void for_each(Fn&& fn) const {
      const size_t n = published.load(std::memory_order_acquire);
      for (size_t i = 0; i < n; ++i) {
        T* entry = slots[i].load(std::memory_order_acquire);
        if (entry != nullptr) {
          fn(*entry);
        }
      }
    }

    /// any thread, wait-free: first live entry satisfying pred(const T&), else nullptr
    template <typename Fn>
    T* find_if(Fn&& pred) const {
      const size_t n = published.load(std::memory_order_acquire);
      for (size_t i = 0; i < n; ++i) {
        T* entry = slots[i].load(std::memory_order_acquire);
        if (entry != nullptr && pred(*entry)) {
          return entry;
        }
      }
      return nullptr;
    }

    constexpr static size_t capacity() { return Capacity; }

   private:
    std::array<std::atomic<T*>, Capacity> slots{};
    std::atomic<size_t> published{ 0 };
  };

}  // namespace other

#endif  // OTHER_CORE_THREAD_SLOT_REGISTRY_HPP

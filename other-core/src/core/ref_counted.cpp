/**
 * \file core/ref_counted.cpp
 */
#include "core/ref_counted.hpp"

#include <atomic>

namespace other {

  /// \note relaxed works from increment b/c you already own a reference to the object,
  //          so you've established a happens-before relationship and the ref is safe to increment
  //        decrement requires acq_rel to avoid two threads racing to decrement count to 0 and then
  //          both seeing a count of 0 after decrement which would lead to double-free
  //        count needs to see the most up-to-date value of ref_count to know when it hits 0, so it needs acquire semantics

  void ref_counted::view_increment() const {
    views.fetch_add(1, std::memory_order_relaxed);
  }

  natural_t ref_counted::view_decrement() const {
    return views.fetch_sub(1, std::memory_order_acq_rel) - 1;
  }

  natural_t ref_counted::increment() {
    return ref_count.fetch_add(1, std::memory_order_relaxed) + 1;
  }

  natural_t ref_counted::decrement() {
    return ref_count.fetch_sub(1, std::memory_order_acq_rel) - 1;
  }

  bool ref_counted::try_increment() {
    natural_t current = ref_count.load(std::memory_order_relaxed);
    while (current != 0) {
      if (ref_count.compare_exchange_weak(current, current + 1, std::memory_order_acq_rel, std::memory_order_relaxed)) {
        return true;
      }
    }
    return false;
  }

  natural_t ref_counted::view_count() const {
    return views.load(std::memory_order_acquire);
  }

  natural_t ref_counted::count() const {
    return ref_count.load(std::memory_order_acquire);
  }

}  // namespace other

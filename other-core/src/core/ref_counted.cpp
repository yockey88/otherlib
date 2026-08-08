/**
 * \file core/ref_counted.cpp
 */
#include "core/ref_counted.hpp"

#include <atomic>

namespace other {

  /// \note increment is relaxed (already own a ref = happens-before established);
  //  decrement needs acq_rel to avoid a double-free race and see the count hit 0

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

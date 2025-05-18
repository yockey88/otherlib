/**
 * \file kernel/ref_counted.cpp
 */
#include "kernel/ref_counted.hpp"

namespace other {

  void RefCounted::ViewIncrement() const {
    views++;
  }

  void RefCounted::ViewDecrement() const {
    views--;
  }

  void RefCounted::Increment() {
    count++;
  }

  void RefCounted::Decrement() {
    count--;
  }

  uint64_t RefCounted::ViewCount() const {
    return views;
  }

  uint64_t RefCounted::Count() const {
    return count;
  }

}  // namespace other

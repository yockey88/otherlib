/**
 * \file core/ref_counted.cpp
 */
#include "core/ref_counted.hpp"

namespace other {

  void ref_counted::view_increment() const {
    views++;
  }

  void ref_counted::view_decrement() const {
    views--;
  }

  void ref_counted::increment() {
    ref_count++;
  }

  void ref_counted::decrement() {
    ref_count--;
  }

  uint64_t ref_counted::view_count() const {
    return views;
  }

  uint64_t ref_counted::count() const {
    return ref_count;
  }

}  // namespace other

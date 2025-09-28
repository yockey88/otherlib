/**
 * \file core/scope.hpp
 **/
#ifndef OTHER_CORE_SCOPE_HPP
#define OTHER_CORE_SCOPE_HPP

#include <memory>

#include "core/arena_allocator.hpp"
#include "core/profiler.hpp"

namespace other {

  template <typename T>
  struct scope_deleter {
    void operator()(T* ptr) const {
      if (ptr != nullptr) {
        arena_allocator<T>{}.free(ptr);
        ptr = nullptr;
      }
    }

    scope_deleter() = default;
    template <typename U>
      requires std::is_base_of_v<T, U>
    scope_deleter(const scope_deleter<U>&) {}
  };

  template <typename T>
  using scope = std::unique_ptr<T, scope_deleter<T>>;

  template <typename T, typename... Args>
    requires requires(Args&&... args) { std::declval<arena_allocator<T>>().allocate(std::forward<Args>(args)...); }
  scope<T> make_scope(Args&&... args) {
    return std::unique_ptr<T, scope_deleter<T>>(arena_allocator<T>{}.allocate(std::forward<Args>(args)...), scope_deleter<T>());
  }

}  // namespace other

#endif  // OTHER_CORE_SCOPE_HPP
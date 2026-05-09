/**
 * \file core/ref.hpp
 */
#ifndef OTHER_CORE_REF_HPP
#define OTHER_CORE_REF_HPP

#include <atomic>
#include <concepts>
#include <type_traits>
#include <utility>

#include "core/arena_allocator.hpp"
#include "core/logger.hpp"
#include "core/ref_counted.hpp"

namespace other {

  template <typename T, typename U>
  concept ref_castable = std::convertible_to<T, U> || std::derived_from<T, U> || std::derived_from<U, T>;

  template <typename T>
  concept ref_type = std::derived_from<T, ref_counted>;

  template <typename T>
  class ref {
   public:
    ref() : object(nullptr) {}

    ref(T* p) : object(p) {
      inc_ref();
    }

    template <typename T2>
      requires std::is_base_of_v<T, T2>
    ref(T2* p) : object((T*)p) {
      inc_ref();
    }

    ref(ref<T>&& other) noexcept {
      T* p = other.object.exchange(nullptr, std::memory_order_acq_rel);
      object.store(p, std::memory_order_release);
    }
    template <typename T2>
      requires std::is_base_of_v<T, T2>
    ref(ref<T2>&& other) noexcept {
      static_assert(std::is_base_of_v<T, T2>, "No viable conversion to construct ref with");
      T2* p = other.object.exchange(nullptr, std::memory_order_acq_rel);
      object.store(reinterpret_cast<T*>(p), std::memory_order_release);
    }
    ref& operator=(ref<T>&& other) noexcept {
      if (this != &other) {
        T* new_p = other.object.exchange(nullptr, std::memory_order_acq_rel);
        T* old_p = object.exchange(new_p, std::memory_order_acq_rel);
        dec_ref_ptr(old_p);
      }
      return *this;
    }
    template <typename T2>
      requires std::is_base_of_v<T, T2>
    ref& operator=(ref<T2>&& other) noexcept {
      static_assert(std::is_base_of_v<T, T2>, "No viable conversion to construct ref with");
      T2* new_p = other.object.exchange(nullptr, std::memory_order_acq_rel);
      T* old_p = object.exchange(reinterpret_cast<T*>(new_p), std::memory_order_acq_rel);
      dec_ref_ptr(old_p);
      return *this;
    }

    ref(const ref<T>& other) {
      object.store(other.object.load(std::memory_order_acquire), std::memory_order_release);
      inc_ref();
    }
    template <typename T2>
      requires std::is_base_of_v<T, T2>
    ref(const ref<T2>& other) {
      static_assert(std::is_base_of_v<T, T2>, "No viable conversion to construct ref with");
      object.store(reinterpret_cast<T*>(other.object.load(std::memory_order_acquire)), std::memory_order_release);
      inc_ref();
    }
    ref& operator=(const ref<T>& other) {
      if (this != &other) {
        object.store(other.object.load(std::memory_order_acquire), std::memory_order_release);
        inc_ref();
      }
      return *this;
    }
    template <typename T2>
      requires std::is_base_of_v<T, T2>
    ref& operator=(const ref<T2>& other) {
      static_assert(std::is_base_of_v<T, T2>, "No viable conversion to construct ref with");
      object.store(reinterpret_cast<T*>(other.object.load(std::memory_order_acquire)), std::memory_order_release);
      inc_ref();
      return *this;
    }

    ref& operator=(std::nullptr_t) {
      if (object.load(std::memory_order_acquire) != nullptr) {
        dec_ref_ptr(object.load(std::memory_order_acquire));
      }
      object.store(nullptr, std::memory_order_release);
      return *this;
    }

    virtual ~ref() {
      T* old_p = object.exchange(nullptr, std::memory_order_acq_rel);
      dec_ref_ptr(old_p);
    }

    template <typename U>
      requires ref_castable<T, U>
    static ref<T> clone(const ref<U>& old_ref) {
      if constexpr (std::same_as<T, U>) {
        return ref<T>(old_ref);
      } else {
        U* p = old_ref.object.load(std::memory_order_acquire);
        return ref<T>(reinterpret_cast<T*>(p));
      }
      OTHER_ASSERT(false, "No viable conversion to construct ref with");
    }

    template <typename U>
      requires ref_castable<T, U>
    static ref<U> cast(ref<T> other) {
      T* p = other.object.load(std::memory_order_acquire);
      if (p == nullptr) {
        return nullptr;
      }

      if constexpr (std::is_base_of_v<T, U>) {
        return ref<U>(reinterpret_cast<U*>(other.object.load(std::memory_order_acquire)));
      } else if constexpr (std::is_base_of_v<U, T>) {
        return ref<U>(reinterpret_cast<U*>(other.object.load(std::memory_order_acquire)));
      } else {
        static_assert(ref_castable<T, U>, "No viable cast from ref<T> to ref<U>");
        return nullptr;
      }
    }

    template <typename U>
      requires ref_type<U> && std::is_default_constructible_v<U>
    static ref<std::remove_cvref_t<U>> create() {
      return ref<std::remove_cvref_t<U>>(allocator.allocate());
    }

    template <typename U>
      requires ref_type<U> && std::is_copy_constructible_v<U>
    static ref<std::remove_cvref_t<U>> create(const U& other) {
      return ref<std::remove_cvref_t<U>>(allocator.allocate(other));
    }

    template <typename U>
      requires ref_type<U> && std::is_move_constructible_v<U>
    static ref<std::remove_cvref_t<U>> create(U&& other) {
      return ref<std::remove_cvref_t<U>>(allocator.allocate(std::move(other)));
    }

    template <typename... Args>
      requires std::is_base_of_v<ref_counted, std::remove_cvref_t<T>>
    static ref<std::remove_cvref_t<T>> create(Args&&... args) {
      return ref<std::remove_cvref_t<T>>(arena_allocator<std::remove_cvref_t<T>>{}.allocate(std::forward<Args>(args)...));
    }

    void increment_weak() const {
      if (object.load(std::memory_order_acquire) != nullptr) {
        object.load(std::memory_order_acquire)->view_increment();
      }
    }
    void decrement_weak() const {
      if (object.load(std::memory_order_acquire) != nullptr) {
        object.load(std::memory_order_acquire)->view_decrement();
      }
    }

    operator bool() { return object.load(std::memory_order_acquire) != nullptr; }
    operator bool() const { return object.load(std::memory_order_acquire) != nullptr; }

    T* operator->() { return object.load(std::memory_order_acquire); }
    const T* operator->() const { return object.load(std::memory_order_acquire); }
    T* raw_ptr() { return object.load(std::memory_order_acquire); }
    const T* raw_ptr() const { return object.load(std::memory_order_acquire); }

    T& operator*() { return *operator->(); }
    const T& operator*() const { return *operator->(); }

    size_t count() const {
      T* obj = object.load(std::memory_order_acquire);
      return obj != nullptr ? obj->count() : 0;
    }

    bool operator==(std::nullptr_t) const { return object.load(std::memory_order_acquire) == nullptr; }
    bool operator==(const ref<T>& other) const { return object.load(std::memory_order_acquire) == other.object.load(std::memory_order_acquire); }

   private:
    template <typename U>
    friend class ref;
    template <typename U>
    friend class weak_ref;

    static inline arena_allocator<T> allocator;

    /// requires mutable to call IncRef and DecRef in const contexts
    mutable std::atomic<T*> object;

    void inc_ref() const {
      T* p = object.load(std::memory_order_acquire);
      if (p != nullptr) {
        p->increment();
      }
    }

    void dec_ref() const {
      T* p = object.load(std::memory_order_acquire);
      dec_ref_ptr(p);
    }

    static void dec_ref_ptr(T* ptr) {
      if (ptr == nullptr) {
        return;
      }

      if (ptr->decrement() == 0) {
        std::atomic_thread_fence(std::memory_order_acquire);
        allocator.free(ptr);
      }
    }
  };

  template <typename T, typename... Args>
    requires ref_type<T> && std::constructible_from<T, Args...>
  ref<T> make_ref(Args&&... args) {
    return ref<T>::create(std::forward<Args>(args)...);
  }

  template <typename T>
    requires ref_type<T> && std::is_default_constructible_v<T>
  ref<T> make_ref() {
    return ref<T>::create();
  }

  template <typename T>
    requires ref_type<T> && std::is_copy_constructible_v<T>
  ref<T> make_ref(const T& other) {
    return ref<T>::create(other);
  }

  template <typename T>
    requires ref_type<T> && std::is_move_constructible_v<T>
  ref<T> make_ref(T&& other) {
    return ref<T>::create(std::move(other));
  }

}  // namespace other

#endif  // OTHER_CORE_REF_HPP

/**
 * \file core/weak_ref.hpp
 **/
#ifndef OTHER_CORE_WEAK_REF_HPP
#define OTHER_CORE_WEAK_REF_HPP

#include "core/ref.hpp"

namespace other {

  /// non-owning observer of a ref-counted object:
  ///  - does NOT keep the object logically alive: once the last strong ref releases,
  ///    the weak_ref reports expired and lock() returns null (no resurrection)
  ///  - DOES pin the storage: the allocation (and therefore the destructor) is not
  ///    released until the last weak_ref goes away, so observing expiry is never a
  ///    use-after-free
  template <typename T>
  class weak_ref {
   public:
    weak_ref() : object(nullptr) {}

    weak_ref(const ref<T>& strong_ref) {
      object = strong_ref.object.load(std::memory_order_acquire);
      if (object != nullptr) {
        object->view_increment();
      }
    }

    weak_ref(const weak_ref& other) : object(other.object) {
      if (object != nullptr) {
        object->view_increment();
      }
    }
    weak_ref(weak_ref&& other) noexcept : object(other.object) {
      other.object = nullptr;
    }

    weak_ref& operator=(const weak_ref& other) {
      if (this != &other) {
        /// take the incoming view share before releasing the old one so self-aliasing
        ///  assignment stays balanced
        T* new_p = other.object;
        if (new_p != nullptr) {
          new_p->view_increment();
        }
        T* old_p = object;
        object = new_p;
        ref<T>::dec_view_ptr(old_p);
      }
      return *this;
    }
    weak_ref& operator=(weak_ref&& other) noexcept {
      if (this != &other) {
        T* old_p = object;
        object = other.object;
        other.object = nullptr;
        ref<T>::dec_view_ptr(old_p);
      }
      return *this;
    }

    weak_ref& operator=(std::nullptr_t) {
      T* old_p = object;
      object = nullptr;
      ref<T>::dec_view_ptr(old_p);
      return *this;
    }

    ~weak_ref() {
      ref<T>::dec_view_ptr(object);
    }

    bool expired() const { return object == nullptr || object->count() == 0; }

    operator bool() { return !expired(); }
    operator bool() const { return !expired(); }

    auto operator->() { return lock(*this).operator->(); }
    auto operator->() const { return lock(*this).operator->(); }

    T& operator*() { return *lock(*this); }
    const T& operator*() const { return *lock(*this); }

    bool operator==(const weak_ref& other) const { return object == other.object; }
    bool operator!=(const weak_ref& other) const { return object != other.object; }
    bool operator==(std::nullptr_t) const { return object == nullptr; }
    bool operator!=(std::nullptr_t) const { return object != nullptr; }

    /// null if the object has expired; otherwise a strong ref that is guaranteed valid
    ///  (the count is taken atomically, so a concurrent last-strong-release either loses
    ///   to the lock or makes it return null)
    static ref<T> lock(const weak_ref& weak) {
      T* p = weak.object;
      if (p == nullptr || !p->try_increment()) {
        return nullptr;
      }

      ref<T> result;
      result.object.store(p, std::memory_order_release);
      return result;
    }

    ref<T> lock() const { return lock(*this); }

   private:
    template <typename U>
    friend class ref;
    template <typename U>
    friend class weak_ref;

    T* object;
  };

  template <typename T>
  weak_ref(const ref<T>& strong_ref) -> weak_ref<T>;

  template <typename T>
  weak_ref<T> make_weak_ref(const ref<T>& strong_ref) {
    return weak_ref<T>(strong_ref);
  }

}  // namespace other

#endif  // OTHER_CORE_WEAK_REF_HPP

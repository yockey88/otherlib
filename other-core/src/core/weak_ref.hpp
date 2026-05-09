/**
 * \file core/weak_ref.hpp
 **/
#ifndef OTHER_CORE_WEAK_REF_HPP
#define OTHER_CORE_WEAK_REF_HPP

#include "core/ref.hpp"

namespace other {

  template <typename T>
  class weak_ref {
   public:
    weak_ref(ref<T>& strong_ref) : reference(strong_ref) {
      if (strong_ref != nullptr) {
        strong_ref.increment_weak();
      }
    }
    weak_ref(weak_ref&& other) noexcept : reference(other.reference) {
      other.reference = nullptr;
    }
    weak_ref(const weak_ref& other) : reference(other.reference) {
      if (reference != nullptr) {
        reference.increment_weak();
      }
    }
    weak_ref& operator=(weak_ref&& other) noexcept {
      if (this != &other) {
        reference = other.reference;
        other.reference = nullptr;
      }
      return *this;
    }
    weak_ref& operator=(const weak_ref& other) {
      if (this != &other) {
        if (reference != nullptr) {
          reference.decrement_weak();
        }
        reference = other.reference;
        if (reference != nullptr) {
          reference.increment_weak();
        }
      }
      return *this;
    }
    ~weak_ref() {
      if (reference != nullptr) {
        reference.decrement_weak();
      }
    }

    operator bool() { return reference != nullptr && reference->count() > 0; }
    operator bool() const { return reference != nullptr && reference->count() > 0; }

    auto operator->() { return lock(*this).operator->(); }
    auto operator->() const { return lock(*this).operator->(); }

    T& operator*() { return *lock(*this); }
    const T& operator*() const { return *lock(*this); }

    bool operator==(const weak_ref& other) const { return reference == other.reference; }
    bool operator!=(const weak_ref& other) const { return reference != other.reference; }
    bool operator==(std::nullptr_t) const { return reference == nullptr; }
    bool operator!=(std::nullptr_t) const { return reference != nullptr; }

    static ref<T> lock(const weak_ref& weak) {
      if (weak.reference == nullptr) {
        return nullptr;
      }
      OTHER_ASSERT(weak.reference->count() > 0, "Attempting to lock weak_ref with expired reference");
      return ref<T>(weak.reference.operator->());
    }

   private:
    template <typename U>
    friend class ref;
    template <typename U>
    friend class weak_ref;

    ref<T> reference;
  };

  template <typename T>
  weak_ref(ref<T>& strong_ref) -> weak_ref<T>;

  template <typename T>
  weak_ref<T> make_weak_ref(ref<T>& strong_ref) {
    return weak_ref<T>(strong_ref);
  }

}  // namespace other

#endif  // OTHER_CORE_WEAK_REF_HPP
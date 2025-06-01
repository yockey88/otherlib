/**
 * \file core/ref.hpp
 */
#ifndef OTHER_CORE_REF_HPP
#define OTHER_CORE_REF_HPP

#include <concepts>
#include <utility>

#include "arena_allocator.hpp"
#include "ref_counted.hpp"

namespace other {
  namespace detail {

    void register_reference(void* instance);
    void remove_reference(void* instance);
    bool is_valid_ref(void* instance);
    size_t num_living_references();

  }  // namespace detail

  template <typename T, typename U>
  concept ref_castable = std::convertible_to<T, U> || std::derived_from<T, U> || std::derived_from<U, T>;

  template <typename T>
  concept ref_type = std::derived_from<T, ref_counted>;

  template <typename T>
  class ref {
   public:
    ref() : object(nullptr) {}

    ref(T* p) {
      object = p;
      inc_ref();
    }

    template <typename T2>
      requires std::is_base_of_v<T, T2>
    ref(T2* p) {
      object = p;
      inc_ref();
    }

    ref(ref<T>&& other) noexcept {
      object = other.object;
      other.object = nullptr;
    }
    ref& operator=(ref<T>&& other) noexcept {
      if (this != &other) {
        object = other.object;
        other.object = nullptr;
      }
      return *this;
    }
    template <typename T2>
      requires std::is_base_of_v<T, T2>
    ref(ref<T2>&& other) noexcept {
      static_assert(std::is_base_of_v<T, T2>, "No viable conversion to construct ref with");
      object = reinterpret_cast<T*>(other.object);
      other.object = nullptr;
    }

    ref(const ref<T>& other) {
      object = other.object;
      inc_ref();
    }
    ref& operator=(const ref<T>& other) {
      if (this != &other) {
        object = other.object;
        inc_ref();
      }
      return *this;
    }
    template <typename T2>
      requires std::is_base_of_v<T, T2>
    ref(const ref<T2>& other) {
      static_assert(std::is_base_of_v<T, T2>, "No viable conversion to construct ref with");
      object = reinterpret_cast<T*>(other.object);
      inc_ref();
    }

    virtual ~ref() {
      dec_ref();
    }

    template <typename T2>
    ref& operator=(const ref<T2>& other) {
      static_assert(std::is_base_of_v<T, T2>, "No viable conversion to construct ref with");
      other.inc_ref();
      dec_ref();

      object = reinterpret_cast<T*>(other.object);
      return *this;
    }

    /// ref count stays the same under move assignment
    template <typename T2>
    ref& operator=(ref<T2>&& other) noexcept {
      static_assert(std::is_base_of_v<T, T2>, "No viable conversion to construct ref with");

      object = reinterpret_cast<T*>(other.object);
      other.object = nullptr;

      return *this;
    }

    ref& operator=(std::nullptr_t) {
      if (object != nullptr) {
        dec_ref();
      }
      object = nullptr;
      return *this;
    }

    operator bool() { return object != nullptr; }
    operator bool() const { return object != nullptr; }

    T& operator*() { return *object; }
    const T& operator*() const { return *object; }

    T* operator->() { return object; }
    const T* operator->() const { return object; }

    T* raw_ptr() { return object; }
    const T* raw_ptr() const { return object; }

    size_t count() const {
      if (object != nullptr) {
        return object->count();
      }
      return 0;
    }

    template <typename U>
      requires ref_castable<T, U>
    static ref<T> clone(const ref<U>& old_ref) {
      if constexpr (std::same_as<T, U>) {
        return ref<T>(old_ref);
      } else {
        return ref<T>(reinterpret_cast<T*>(old_ref.object));
      }

      /// Unreachable
      throw std::runtime_error("Invalid cast from ref<U> to ref<T>");
      // throw invalid_ref_cast(typeid(T), typeid(U));
    }

    template <typename... Args>
      requires std::is_base_of_v<ref_counted, std::remove_cvref_t<T>> &&
      requires(Args&&... args) { std::declval<arena_allocator<std::remove_cvref_t<T>>>().allocate(std::forward<Args>(args)...); }
    static ref<std::remove_cvref_t<T>> create(Args&&... args) {
      return ref<std::remove_cvref_t<T>>(arena_allocator<std::remove_cvref_t<T>>{}.allocate(std::forward<Args>(args)...));
    }

    bool operator==(std::nullptr_t) const { return object == nullptr; }
    bool operator==(const ref<T>& other) const { return object == other.object; }

   private:
    static inline arena_allocator<T> allocator;

    /// requires mutable to call IncRef and DecRef in const contexts
    mutable T* object;

    /// for direct referncing in cases where we don't want to increment the reference count
    ref(T* p, bool) {
      object = p;
    }

    void inc_ref() const {
      if (object != nullptr) {
        object->increment();
        detail::register_reference(object);
      }
    }

    void dec_ref() const {
      if (object != nullptr) {
        object->decrement();

        if (object->count() == 0) {
          // OE_ASSERT(detail::IsValidRef(object), "Invalid reference detected.");
          // OE_ASSERT(detail::NumberOfLivingReferences() > 0, "No living references detected to remove.");

          // OE_ASSERT(object->ViewCount() == 0, "Attempting to delete object with active views. Type = {} \\ views = {}", typeid(T).name(), object->ViewCount());

          detail::remove_reference(object);
          allocator.free(object);
          object = nullptr;
        }
      }
    }

    template <typename U>
    friend class ref;
  };

  template <typename T, typename... Args>
    requires ref_type<T> && std::constructible_from<T, Args...>
  ref<T> make_ref(Args&&... args) {
    return ref<T>::create(std::forward<Args>(args)...);
  }

}  // namespace other

#endif  // OTHER_CORE_REF_HPP

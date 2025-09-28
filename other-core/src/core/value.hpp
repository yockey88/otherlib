/**
 * \file core/value.hpp
 **/
#ifndef OTHER_CORE_VALUE_HPP
#define OTHER_CORE_VALUE_HPP

#include <mutex>

#include "core/defines.hpp"
#include "core/logger.hpp"
#include "core/ref.hpp"
#include "core/ref_counted.hpp"

#include "arena_allocator.hpp"


namespace other {

  class value_storage : public ref_counted {
   public:
    value_storage() = default;
    virtual ~value_storage() = default;

    virtual size_t size() const = 0;
    virtual value_type val_type() const = 0;

    template <typename T>
    T* unchecked_ptr_unwrap() { return reinterpret_cast<T*>(data()); }
    template <typename T>
    const T* unchecked_ptr_unwrap() const { return reinterpret_cast<const T*>(data()); }

    template <typename T>
    T& unchecked_unwrap() { return *unchecked_ptr_unwrap<T>(); }
    template <typename T>
    const T& unchecked_unwrap() const { return *unchecked_ptr_unwrap<const T>(); }

   protected:
    friend class value;
    template <typename T>
    friend class value_storage_impl;

    virtual void* data() = 0;
    virtual const void* data() const = 0;
    virtual void set_data(void* data) = 0;
  };

  template <typename T>
  class value_storage_impl : public value_storage {
   public:
    value_storage_impl() {
      static_assert(std::same_as<T, void*>, "value_storage_impl default constructor must be used for opaque_handle type!");
      type_size = sizeof(void*);
    }

    value_storage_impl(void* opaque_data) {
      static_assert(std::same_as<T, void*>, "value_storage_impl default constructor must be used for opaque_handle type!");
      type_size = sizeof(void*);

      object = opaque_data;

      if (object == nullptr) {
        // OE_ASSERT(data != nullptr, "Opaque data pointer is null!");
      }
    }

    value_storage_impl(const T& value) {
      object = allocator.allocate(value);

      if constexpr (std::same_as<T, std::string> || std::same_as<T, std::string_view>) {
        type_size = value.size();
      } else {
        type_size = sizeof(T);
      }
    }
    value_storage_impl(T&& value) {
      object = allocator.allocate(std::move(value));

      if constexpr (std::same_as<T, std::string> || std::same_as<T, std::string_view>) {
        type_size = value.size();
      } else {
        type_size = sizeof(T);
      }
    }

    value_storage_impl(T* value_ptr) {
      OTHER_ASSERT(value_ptr != nullptr, "Value pointer is null!");
      object = allocator.allocate(*value_ptr);

      if constexpr (std::same_as<T, std::string> || std::same_as<T, std::string_view>) {
        type_size = value_ptr->size();
      } else {
        type_size = sizeof(T);
      }
    }

    ~value_storage_impl() {
      if (val_type() != value_type::OPAQUE_HANDLE) {
        allocator.free(object);
      }
      object = nullptr;
      type_size = 0;
    }

    value_storage_impl(value_storage_impl&& other) = delete;
    value_storage_impl(const value_storage_impl& other) {
      object = other.data();
      type_size = other.size();
    }
    value_storage_impl& operator=(value_storage_impl&& other) = delete;
    value_storage_impl& operator=(const value_storage_impl& other) {
      object = other.data();
      type_size = other.size();
      return *this;
    }

   private:
    void* data() override { return this->object; }
    const void* data() const override { return this->object; }

    void set_data(void* data) override {
      if (val_type() != value_type::OPAQUE_HANDLE) {
        if (T* old_object = object; old_object != nullptr) {
          allocator.free(old_object);
        }
      }

      if (data == nullptr) {
        object = nullptr;
      } else {
        object = static_cast<T*>(data);
      }
    }

    size_t size() const override {
      if constexpr (std::is_same_v<T, std::string> || std::is_same_v<T, std::string_view>) {
        return type_size;  // For strings, size is determined by the string's size
      } else {
        return sizeof(T);  // For other types, size is fixed
      }
    }
    value_type val_type() const override { return get_value_type<T>(); }

    arena_allocator<T> allocator;

    T* object = nullptr;
    size_t type_size = 0;
  };

  class value {
   public:
    value() {}

    value(const value& other);
    value& operator=(const value& other);
    template <typename T>
      requires(!std::is_pointer_v<T> && !std::is_same_v<T, void*>)
    value(const T& value) {
      storage = make_ref<value_storage_impl<T>>(value);
    }

    value(value&& other);
    value& operator=(value&& other);
    template <typename T>
      requires(!std::is_pointer_v<T> && !std::is_same_v<T, void*>)
    value(T&& value) {
      storage = make_ref<value_storage_impl<T>>(std::move(value));
    }

    value(std::nullptr_t) { storage = nullptr; }
    template <typename T>
      requires std::is_pointer_v<T>
    value(T* value_ptr) {
      if constexpr (std::is_same_v<T, void*>) {
        static_assert(!std::is_same_v<T, void*>, "Use create_opaque_handle for opaque handles!");
      }
      storage = make_ref<value_storage_impl<T>>(value_ptr);
    }

    ~value();

    static value create_opaque_handle(void* opaque_data);

    bool is_empty() const;
    void clear();

    template <typename T>
    operator T&() { return unwrap_as<T>(); }
    template <typename T>
    operator const T&() const { return unwrap_as<T>(); }

    size_t size() const;
    value_type type() const;

    void aquire();
    void release();

   private:
    std::mutex mutex;

    ref<value_storage> storage = nullptr;

    template <typename T>
    T& unwrap_as() {
      check<T>();
      return storage->unchecked_unwrap<T>();
    }

    template <typename T>
    const T& unwrap_as() const {
      check<T>();
      return storage->unchecked_unwrap<const T>();
    }

    template <typename T>
    T* ptr() {
      if (storage == nullptr || storage->size() != sizeof(T) || storage->val_type() != get_value_type<T>()) {
        return nullptr;
      }
      return storage->unchecked_ptr_unwrap<T>();
    }

    template <typename T>
    const T* ptr() const {
      if (storage == nullptr || storage->size() != sizeof(T) || storage->val_type() != get_value_type<T>()) {
        return nullptr;
      }
      return storage->unchecked_ptr_unwrap<const T>();
    }

    template <typename T>
    void check() const {
      OTHER_ASSERT(storage != nullptr, "Storage is null!");
      OTHER_ASSERT(storage->size() >= sizeof(T), "Size mismatch! stored type: {}, requested type: {}", storage->size(), sizeof(T));
      OTHER_ASSERT(storage->val_type() == get_value_type<T>(), "Value type mismatch! stored type: {}, requested type: {}", storage->val_type(), get_value_type<T>());
    }
  };

}  // namespace other

#endif  // OTHER_CORE_VALUE_HPP
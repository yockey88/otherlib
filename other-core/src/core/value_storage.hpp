/**
 * \file core/value_storage.hpp
 **/
#ifndef OTHER_CORE_CORE_VALUE_STORAGE_HPP
#define OTHER_CORE_CORE_VALUE_STORAGE_HPP

#include <cstring>

#include "core/arena_allocator.hpp"
#include "core/defines.hpp"
#include "core/logger.hpp"
#include "core/ref.hpp"
#include "core/ref_counted.hpp"

namespace other {

  struct value_storage : public ref_counted {
    value_storage() = default;
    virtual ~value_storage() = default;

    virtual size_t size() const = 0;
    virtual value_type val_type() const = 0;

    template <typename T>
    static ref<T> create_storage(const T& value);

    template <typename T>
      requires(!is_opaque_pointer<T>)
    T* unchecked_ptr_unwrap() { return reinterpret_cast<T*>(data()); }
    template <typename T>
      requires(!is_opaque_pointer<T>)
    const T* unchecked_ptr_unwrap() const { return reinterpret_cast<const T*>(data()); }

    template <typename T>
      requires(!is_string_type<T> && !is_opaque_pointer<T>)
    T& unchecked_unwrap() { return *unchecked_ptr_unwrap<T>(); }
    template <typename T>
      requires(!is_string_type<T> && !is_opaque_pointer<T>)
    const T& unchecked_unwrap() const { return *unchecked_ptr_unwrap<const T>(); }

    template <typename T>
      requires(is_string_type<T>)
    std::string unchecked_unwrap() { return unchecked_string_unwrap(); }
    template <typename T>
      requires(is_string_type<T>)
    std::string unchecked_unwrap() const { return unchecked_string_unwrap(); }

    void* unwrap_opaque_handle();

    std::string unchecked_string_unwrap() const;

    friend class value;
    template <typename T>
    friend class value_storage_impl;

    virtual void* data() = 0;
    virtual const void* data() const = 0;
    virtual const void* memory() const = 0;
    virtual void reallocate(size_t new_size) = 0;
    virtual void overwrite_data(const void* data, size_t sz) = 0;
  };

  template <typename T>
  class value_storage_impl : public value_storage {
   public:
    value_storage_impl() {
      static_assert(is_opaque_pointer<T>, "value_storage_impl default constructor must be used for opaque_handle type!");
      type_size = sizeof(void*);
    }

    value_storage_impl(const T& value) {
      arena_allocator<T> allocator;

      if constexpr (is_string_type<T>) {
        size_t size = 0;
        const char* cstr = nullptr;
        if constexpr (is_character_array<T>) {
          cstr = value;
          size = std::strlen(cstr) + 1;
        } else {
          cstr = value.data();
          size = value.size();
        }
        raw_data = allocator.allocate_bytes(size);
        std::memcpy(raw_data, cstr, size);
        type_size = size;
      } else {
        object = allocator.allocate(value);
        type_size = sizeof(T);
      }
    }

    value_storage_impl(T&& value) {
      arena_allocator<T> allocator;

      if constexpr (is_string_type<T>) {
        size_t size;
        const char* cstr = nullptr;
        if constexpr (is_character_array<T>) {
          cstr = value;
          size = std::strlen(cstr) + 1;
        } else {
          cstr = value.data();
          size = value.size();
        }
        raw_data = allocator.allocate_bytes(size);
        std::memcpy(raw_data, cstr, size);
        type_size = size;
      } else {
        object = allocator.allocate(std::move(value));
        type_size = sizeof(T);
      }
    }

    ~value_storage_impl() {
      arena_allocator<T> allocator;

      if (val_type() != value_type::OPAQUE_HANDLE) {
        if (val_type() == value_type::STRING) {
          allocator.free_bytes(raw_data, type_size);
        } else {
          allocator.free(object);
        }
      }
      raw_data = nullptr;
      object = nullptr;
      type_size = 0;
    }

    value_storage_impl(value_storage_impl&& other) = delete;
    value_storage_impl(const value_storage_impl& other) = delete;
    value_storage_impl& operator=(value_storage_impl&& other) = delete;
    value_storage_impl& operator=(const value_storage_impl& other) = delete;

    void* data() override {
      if (val_type() == value_type::STRING || val_type() == value_type::OPAQUE_HANDLE) {
        return raw_data;
      } else {
        return this->object;
      }
    }

    const void* data() const override {
      if (val_type() == value_type::STRING || val_type() == value_type::OPAQUE_HANDLE) {
        return raw_data;
      } else {
        return this->object;
      }
    }

    const void* memory() const override {
      if (val_type() == value_type::STRING || val_type() == value_type::OPAQUE_HANDLE) {
        return raw_data;
      } else {
        return this->object;
      }
    }

    void reallocate(size_t new_size) override {
      arena_allocator<T> allocator;

      if constexpr (is_string_type<T>) {
        OTHER_ASSERT(new_size > 0, "New size must be greater than zero for string reallocation!");

        void* new_raw_data = allocator.allocate_bytes(new_size);
        size_t copy_size = std::min(type_size, new_size);
        std::memcpy(new_raw_data, raw_data, copy_size);

        allocator.free_bytes(raw_data, type_size);
        raw_data = new_raw_data;
        type_size = new_size;
      } else {
        T* new_object = allocator.allocate();
        *new_object = *object;
        allocator.free(object);
        object = new_object;
      }
    }

    void overwrite_data(const void* data, size_t sz) override {
      if (val_type() == value_type::OPAQUE_HANDLE) {
        OTHER_ASSERT(sz == sizeof(void*), "Size mismatch in overwrite_data for opaque handle type!");
        raw_data = const_cast<void*>(data);
      } else if (val_type() == value_type::STRING) {
        OTHER_ASSERT(sz <= type_size, "New string size exceeds allocated size in overwrite_data!");
        std::ranges::fill(std::span(reinterpret_cast<char*>(raw_data), type_size), 0);
        std::memcpy(raw_data, data, sz);
      } else {
        OTHER_ASSERT(sz == sizeof(T), "Size mismatch in overwrite_data for non-string type!");
        std::memcpy(object, data, sz);
      }
    }

   private:
    size_t size() const override {
      if constexpr (is_string_type<T>) {
        return type_size;  // For strings, size is determined by the string's size
      } else {
        return sizeof(T);  // For other types, size is fixed
      }
    }
    value_type val_type() const override { return get_value_type<T>(); }

    /// used only for string types and opaque handles,
    ///   SHOULD BE TREATED WITH CARE !
    void* raw_data = nullptr;
    T* object = nullptr;
    size_t type_size = 0;
  };

}  // namespace other

#endif  // OTHER_CORE_CORE_VALUE_STORAGE_HPP
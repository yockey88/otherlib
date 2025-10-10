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
      requires(!std::is_same_v<T, std::string> && !std::is_same_v<T, std::string_view>)
    T& unchecked_unwrap() { return *unchecked_ptr_unwrap<T>(); }
    template <typename T>
      requires(!std::is_same_v<T, std::string> && !std::is_same_v<T, std::string_view>)
    const T& unchecked_unwrap() const { return *unchecked_ptr_unwrap<const T>(); }

    template <typename T>
      requires(std::is_same_v<T, std::string> || std::is_same_v<T, std::string_view>)
    std::string unchecked_unwrap() { return unchecked_string_unwrap(); }
    template <typename T>
      requires(std::is_same_v<T, std::string> || std::is_same_v<T, std::string_view>)
    std::string unchecked_unwrap() const { return unchecked_string_unwrap(); }

    std::string unchecked_string_unwrap() const {
      const char* str_data = reinterpret_cast<const char*>(memory());
      OTHER_ASSERT(str_data != nullptr, "String data pointer is null!");
      return std::string(str_data, size());
    }

   protected:
    friend class value;
    template <typename T>
    friend class value_storage_impl;

    virtual void* data() = 0;
    virtual const void* data() const = 0;
    virtual const void* memory() const = 0;
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
      raw_data = opaque_data;

      if (object == nullptr) {
        // OE_ASSERT(data != nullptr, "Opaque data pointer is null!");
      }
    }

    value_storage_impl(const T& value) {
      if constexpr (std::same_as<T, std::string> || std::same_as<T, std::string_view>) {
        raw_data = allocator.allocate_bytes(value.size());
        std::memcpy(raw_data, value.data(), value.size());
        type_size = value.size();
      } else {
        object = allocator.allocate(value);
        type_size = sizeof(T);
      }
    }

    value_storage_impl(T&& value) {
      if constexpr (std::same_as<T, std::string> || std::same_as<T, std::string_view>) {
        raw_data = allocator.allocate_bytes(value.size());
        std::memcpy(raw_data, value.data(), value.size());
        type_size = value.size();
      } else {
        object = allocator.allocate(std::move(value));
        type_size = sizeof(T);
      }
    }

    value_storage_impl(T* value_ptr) {
      OTHER_ASSERT(value_ptr != nullptr, "Value pointer is null!");

      if constexpr (std::same_as<T, std::string> || std::same_as<T, std::string_view>) {
        raw_data = allocator.allocate_bytes(value_ptr->size());
        std::memcpy(raw_data, value_ptr->data(), value_ptr->size());
        type_size = value_ptr->size();
      } else {
        object = allocator.allocate(*value_ptr);
        type_size = sizeof(T);
      }
    }

    ~value_storage_impl() {
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

   private:
    void* data() override {
      if (val_type() == value_type::STRING) {
        return raw_data;
      }
      return this->object;
    }
    const void* data() const override {
      if (val_type() == value_type::STRING) {
        return raw_data;
      }
      return this->object;
    }
    const void* memory() const override {
      return raw_data;
    }

    void set_data(void* data) override {
      if (val_type() != value_type::OPAQUE_HANDLE) {
        if (T* old_object = object; old_object != nullptr) {
          allocator.free(old_object);
        }
      } else if (val_type() == value_type::STRING) {
        if (raw_data != nullptr) {
          allocator.free_bytes(raw_data, type_size);
        }
      }

      if (data == nullptr) {
        object = nullptr;
      } else {
        if constexpr (std::is_same_v<T, std::string> || std::is_same_v<T, std::string_view>) {
          raw_data = data;
          type_size = std::strlen(reinterpret_cast<const char*>(data));
        } else {
          object = static_cast<T*>(data);
        }
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

    /// used only for string types and opaque handles,
    ///   SHOULD BE TREATED WITH CARE !
    void* raw_data = nullptr;
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

    template <>
    std::string& unwrap_as<std::string>() {
      check<std::string>();
      static thread_local std::string temp_string;
      temp_string = storage->unchecked_string_unwrap();
      return temp_string;
    }

    template <typename T>
    const T& unwrap_as() const {
      check<T>();
      return storage->unchecked_unwrap<const T>();
    }

    template <>
    const std::string& unwrap_as<std::string>() const {
      check<std::string>();
      static thread_local std::string temp_string;
      temp_string = storage->unchecked_string_unwrap();
      return temp_string;
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
      if constexpr (std::is_same_v<T, std::string>) {
        OTHER_ASSERT(storage->val_type() == value_type::STRING, "Value type mismatch! stored type: {}, requested type: {}", storage->val_type(), get_value_type<T>());
        const char* str_data = reinterpret_cast<const char*>(storage->data());
        OTHER_ASSERT(str_data != nullptr, "String data pointer is null!");
        OTHER_ASSERT(std::strlen(str_data) == storage->size(), "String size mismatch! stored size: {}, requested size: {}", storage->size(), std::strlen(str_data));
      } else {
        OTHER_ASSERT(storage->size() >= sizeof(T), "Size mismatch! stored type: {}, requested type: {}", storage->size(), sizeof(T));
      }
      OTHER_ASSERT(storage->val_type() == get_value_type<T>(), "Value type mismatch! stored type: {}, requested type: {}", storage->val_type(), get_value_type<T>());
    }
  };

}  // namespace other

#endif  // OTHER_CORE_VALUE_HPP
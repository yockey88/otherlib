/**
 * \file core/value.hpp
 **/
#ifndef OTHER_CORE_VALUE_HPP
#define OTHER_CORE_VALUE_HPP

#include <mutex>
#include <string>
#include <type_traits>

#include "core/defines.hpp"
#include "core/enum_formatter.hpp"
#include "core/logger.hpp"
#include "core/ref.hpp"
#include "core/value_storage.hpp"

namespace other {

  class value {
   public:
    value() {}

    value(const value& other);
    value& operator=(const value& other);
    template <typename T>
      requires is_acceptable_value_type<T>
    value(const T& value) {
      /// small optimization: reuse existing storage if possible
      if (storage != nullptr && storage->val_type() == get_value_type<T>()) {
        storage->overwrite_data(&value, sizeof(T));
      } else {
        storage = make_ref<value_storage_impl<T>>(value);
      }
    }

    value(value&& other);
    value& operator=(value&& other);

    value(std::nullptr_t) { storage = nullptr; }
    template <typename T>
      requires std::is_pointer_v<T>
    value(T* value_ptr) {
      if constexpr (std::is_same_v<T, void*>) {
        static_assert(!std::is_same_v<T, void*>, "Use create_opaque_handle for opaque handles!");
      }
      storage = make_ref<value_storage_impl<T>>(value_ptr);
    }

    value(void* data, size_t sz, value_type val_type);

    ~value();

    static value create_opaque_handle(void* opaque_data);

    std::string to_string() const;

    bool is_empty() const;
    void clear();

    template <typename T>
      requires not_string_buffer_or_pointer<T>
    operator T&() { return unwrap_as<T>(); }
    template <typename T>
      requires not_string_buffer_or_pointer<T>
    operator const T&() const { return unwrap_as<T>(); }

    template <typename T>
      requires is_string_type<T>
    operator T() {
      return as_string();
    }
    template <typename T>
      requires is_string_type<T>
    operator T() const {
      return as_string();
    }

    template <typename T>
      requires is_byte_buffer_type<T>
    operator T() const {
      auto byte_span = as_byte_buffer();
      return std::vector<uint8_t>(byte_span.begin(), byte_span.end());
    }

    std::string as_string() {
      check<std::string>();
      return storage->unchecked_string_unwrap();
    }
    std::string as_string() const {
      check<std::string>();
      return storage->unchecked_string_unwrap();
    }
    std::span<const uint8_t> as_byte_buffer() const {
      check<std::span<const uint8_t>>();
      return storage->unchecked_byte_buffer_unwrap();
    }

    operator void*() {
      check<void*>();
      return storage->unwrap_opaque_handle();
    }

    size_t size() const;
    value_type type() const;

    void acquire();
    void release();

    value_storage& get_mutable_storage() {
      OTHER_ASSERT(storage != nullptr, "Attempted to access mutable storage of an empty value!");
      return *storage;
    }
    const value_storage& read_storage() const {
      OTHER_ASSERT(storage != nullptr, "Attempted to read from an empty value storage!");
      return *storage;
    }

   private:
    std::mutex mutex;

    ref<value_storage> storage = nullptr;

    template <typename T>
      requires not_string_buffer_or_pointer<T>
    T& unwrap_as() {
      check<T>();
      return storage->unchecked_unwrap<T>();
    }
    template <typename T>
      requires not_string_buffer_or_pointer<T>
    const T& unwrap_as() const {
      check<T>();
      return storage->unchecked_unwrap<const T>();
    }

    template <typename T>
      requires(is_pointer_type<T>)
    T* ptr() {
      if (storage == nullptr || storage->size() != sizeof(T) || storage->val_type() != get_value_type<T>()) {
        return nullptr;
      }
      return storage->unchecked_ptr_unwrap<T>();
    }

    template <typename T>
      requires(is_pointer_type<T> || is_byte_buffer_type<T>)
    const T* ptr() const {
      if (storage == nullptr || storage->size() != sizeof(T) || storage->val_type() != get_value_type<T>()) {
        return nullptr;
      }
      return storage->unchecked_ptr_unwrap<const T>();
    }

    template <typename T>
    void check() const {
      OTHER_ASSERT(storage != nullptr, "Storage is null!");
      if constexpr (std::is_same_v<std::remove_cvref_t<T>, void*>) {
        OTHER_ASSERT(storage->val_type() == value_type::OPAQUE_HANDLE, "Value type mismatch! stored type: {}, requested type: {}", storage->val_type(), get_value_type<T>());
      } else if constexpr (std::is_same_v<std::remove_cvref_t<T>, std::string>) {
        OTHER_ASSERT(storage->val_type() == value_type::STRING, "Value type mismatch! stored type: {}, requested type: {}", storage->val_type(), get_value_type<T>());
        const char* str_data = reinterpret_cast<const char*>(storage->data());
        OTHER_ASSERT(str_data != nullptr, "String data pointer is null!");
      } else if constexpr (is_byte_buffer_type<T>) {
        OTHER_ASSERT(storage->val_type() == value_type::BYTE_BUFFER, "Value type mismatch! stored type: {}, requested type: {}", storage->val_type(), get_value_type<T>());
        const uint8_t* data_ptr = reinterpret_cast<const uint8_t*>(storage->data());
        OTHER_ASSERT(data_ptr != nullptr, "Byte buffer data pointer is null!");
      } else {
        OTHER_ASSERT(storage->size() >= sizeof(T), "Size mismatch! stored type: {}, requested type: {}", storage->size(), sizeof(T));
      }
      OTHER_ASSERT(storage->val_type() == get_value_type<T>(), "Value type mismatch! stored type: {}, requested type: {}", storage->val_type(), get_value_type<T>());
    }
  };

  namespace detail {

    template <typename T>
    T unpack_value(const value& val) {
      if constexpr (is_string_type<T>) {
        return val.as_string();
      } else {
        return static_cast<T>(val);
      }
    }

    template <typename... Args, std::size_t... Is>
    std::tuple<Args...> unpack_args_impl(const std::span<value> args, std::index_sequence<Is...>) {
      return std::make_tuple(unpack_value<Args>(args[Is])...);
    }

    template <typename... Args>
    std::tuple<Args...> unpack_args(const std::span<value> args) {
      return unpack_args_impl<Args...>(args, std::make_index_sequence<sizeof...(Args)>{});
    }

  }  // namespace detail

}  // namespace other

#endif  // OTHER_CORE_VALUE_HPP
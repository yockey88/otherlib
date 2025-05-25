/**
 * \file core/value.hpp
 **/
#ifndef OTHER_CORE_VALUE_HPP
#define OTHER_CORE_VALUE_HPP

#include <mutex>
#include <string>
#include <string_view>

#include "arena_allocator.hpp"
#include "ref.hpp"
#include "ref_counted.hpp"

namespace other {

  enum value_type {
    EMPTY_TYPE,  // Void , null , nil ,etc...

    /// primitive types
    OEBOOL,
    CHAR,
    INT8,
    INT16,
    INT32,
    INT64,
    UINT8,
    UINT16,
    UINT32,
    UINT64,
    FLOAT,
    DOUBLE,
    STRING,

    /// engine types
    VEC2,
    VEC3,
    VEC4,

    MAT2,
    MAT3,
    MAT4,

    SAMPLER2D,
    SAMPLER2D_ARRAY,

    ASSET,
    ENTITY,

    /// user types
    USER_TYPE,
    OPAQUE_HANDLE,
  };

  template <typename T>
  static constexpr value_type get_value_type() {
    if constexpr (std::is_same_v<T, bool>) {
      return value_type::OEBOOL;
    } else if constexpr (std::is_same_v<T, char>) {
      return value_type::CHAR;
    } else if constexpr (std::is_same_v<T, std::string> || std::is_same_v<T, std::string_view>) {
      return value_type::STRING;
    } else if constexpr (std::is_same_v<T, int8_t>) {
      return value_type::INT8;
    } else if constexpr (std::is_same_v<T, int16_t>) {
      return value_type::INT16;
    } else if constexpr (std::is_same_v<T, int32_t>) {
      return value_type::INT32;
    } else if constexpr (std::is_same_v<T, int64_t>) {
      return value_type::INT64;
    } else if constexpr (std::is_same_v<T, uint8_t>) {
      return value_type::UINT8;
    } else if constexpr (std::is_same_v<T, uint16_t>) {
      return value_type::UINT16;
    } else if constexpr (std::is_same_v<T, uint32_t>) {
      return value_type::UINT32;
    } else if constexpr (std::is_same_v<T, uint64_t>) {
      return value_type::UINT64;
    } else if constexpr (std::is_same_v<T, float>) {
      return value_type::FLOAT;
    } else if constexpr (std::is_same_v<T, double>) {
      return value_type::DOUBLE;
      // } else if constexpr (std::is_same_v<T, glm::vec2>) {
      //   return value_type::VEC2;
      // } else if constexpr (std::is_same_v<T, glm::vec3>) {
      //   return value_type::VEC3;
      // } else if constexpr (std::is_same_v<T, glm::vec4>) {
      //   return value_type::VEC4;
      // } else if constexpr (std::is_same_v<T, glm::mat2>) {
      //   return value_type::MAT2;
      // } else if constexpr (std::is_same_v<T, glm::mat3>) {
      //   return value_type::MAT3;
      // } else if constexpr (std::is_same_v<T, glm::mat4>) {
      //   return value_type::MAT4;
    } else if constexpr (std::is_same_v<T, void*>) {
      return value_type::OPAQUE_HANDLE;
    } else {
      return value_type::USER_TYPE;
    }
  }

  class value_storage : public ref_counted {
   public:
    value_storage() = default;
    virtual ~value_storage() = default;

    virtual void*& data() = 0;
    virtual const void* data() const = 0;

    virtual size_t size() const = 0;

    template <typename T>
    T& UncheckedUnwrap() { return *reinterpret_cast<T*>(data()); }
    template <typename T>
    const T& UncheckedUnwrap() const { return *reinterpret_cast<const T*>(data()); }

    value_type type = value_type::EMPTY_TYPE;

   protected:
    friend class Value;
  };

  template <typename T>
  class value_storage_impl : public value_storage {
   public:
    value_storage_impl() {
      static_assert(std::same_as<T, void*>, "ValueStorageImpl must be specialized for non-void types to use the default constructor!");
      type_size = sizeof(void*);
      type = value_type::OPAQUE_HANDLE;
    }

    value_storage_impl(void* opaque_data) {
      static_assert(std::same_as<T, void*>, "ValueStorageImpl must be specialized for non-void types to use the default constructor!");
      type_size = sizeof(void*);
      type = value_type::OPAQUE_HANDLE;

      object = opaque_data;

      if (object == nullptr) {
        // OE_ASSERT(data != nullptr, "Opaque data pointer is null!");
      }
    }

    value_storage_impl(const T& value) {
      object = allocator.allocate(value);
      type = get_value_type<T>();

      if constexpr (std::same_as<T, std::string> || std::same_as<T, std::string_view>) {
        type_size = value.size();
      } else {
        type_size = sizeof(T);
      }
    }

    value_storage_impl(T* value_ptr) {
      OE_ASSERT(value_ptr != nullptr, "Value pointer is null!");
      object = allocator.allocate(*value_ptr);
      type = get_value_type<T>();

      if constexpr (std::same_as<T, std::string> || std::same_as<T, std::string_view>) {
        type_size = value_ptr->size();
      } else {
        type_size = sizeof(T);
      }
    }

    ~value_storage_impl() {
      if (type != value_type::OPAQUE_HANDLE) {
        allocator.free(object);
      }
      object = nullptr;
      type_size = 0;

      type = value_type::EMPTY_TYPE;
    }

    value_storage_impl(value_storage_impl&& other) = delete;
    value_storage_impl(const value_storage_impl& other) {
      object = other.data;
      type_size = other.size;
      type = other.type;
    }
    value_storage_impl& operator=(value_storage_impl&& other) = delete;
    value_storage_impl& operator=(const value_storage_impl& other) {
      object = other.data;
      type_size = other.size;
      type = other.type;
      return *this;
    }

    void*& data() override { return this->object; }
    const void* data() const override { return this->object; }

    size_t size() const override { return this->type_size; }
    value_type val_type() const { return this->type; }

   private:
    arena_allocator<T> allocator;
    T* object = nullptr;
    size_t type_size = 0;
  };

  class value {
   public:
    value() {}

    template <typename T>
    value(const T& value) {
      storage = NewRef<value_storage_impl<T>>(value);
    }

    template <typename T>
    value(T* value_ptr) {
      storage = NewRef<value_storage_impl<T>>(value_ptr);
    }

    ~value();

    value(value&& other);
    value(const value& other);
    value& operator=(value&& other);
    value& operator=(const value& other);

    static value create_opaque_handle(void* opaque_data);

    bool is_empty() const;
    void clear();

    template <typename T>
    T& unwrap_as() {
      // OE_ASSERT(storage != nullptr, "Storage is null!");
      // OE_ASSERT(storage->value_type != value_type::EMPTY_TYPE, "Value is empty!");
      // OE_ASSERT(storage->value_type != value_type::OPAQUE_HANDLE, "Value is opaque!");
      // OE_ASSERT(storage->value_type == GetValueType<T>(), "Value type mismatch!");
      // OE_ASSERT(storage->data != nullptr, "Data is null!");
      return *std::launder(static_cast<T*>(storage->data()));
    }

    template <typename T>
    const T& unwrap_as() const {
      // OE_ASSERT(storage != nullptr, "Storage is null!");
      // OE_ASSERT(storage->value_type != value_type::EMPTY_TYPE, "Value is empty!");
      // OE_ASSERT(storage->value_type != value_type::OPAQUE_HANDLE, "Value is opaque!");
      // OE_ASSERT(storage->value_type == GetValueType<T>(), "Value type mismatch!");
      // OE_ASSERT(storage->data != nullptr, "Data is null!");
      return *std::launder(static_cast<const T*>(storage->data()));
    }

    template <typename T>
    T* ptr() {
      // OE_ASSERT(storage != nullptr, "Storage is null!");
      if (storage->type == value_type::OPAQUE_HANDLE) {
        return std::launder(static_cast<T*>(storage->data()));
      }

      // OE_ASSERT(storage->value_type == GetValueType<T>(), "Value type mismatch!");
      // OE_ASSERT(storage->data != nullptr, "Data is null!");
      return std::launder(static_cast<T*>(storage->data()));
    }

    template <typename T>
    const T* ptr() const {
      // OE_ASSERT(storage != nullptr, "Storage is null!");
      if (storage->type == value_type::OPAQUE_HANDLE) {
        return std::launder(static_cast<T*>(storage->data()));
      }

      // OE_ASSERT(storage->value_type == GetValueType<T>(), "Value type mismatch!");
      // OE_ASSERT(storage->data != nullptr, "Data is null!");
      return std::launder(static_cast<T*>(storage->data()));
    }

    template <typename T>
    operator T&() { return unwrap_as<T>(); }
    template <typename T>
    operator const T&() const { return unwrap_as<T>(); }

    template <typename T>
    T& operator*() { return *this; }
    template <typename T>
    const T& operator*() const { return *this; }

    size_t size() const;
    value_type type() const;

    void aquire();
    void release();

   private:
    friend struct ValueReference;
    friend class Registers;

    std::mutex mutex;

    ref<value_storage> storage;
  };

}  // namespace other

#endif  // OTHER_CORE_VALUE_HPP
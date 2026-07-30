/**
 * \file serialization/encoding/binary_encoder.hpp
 **/
#ifndef OTHERLIB_SERIALIZATION_ENCODING_BINARY_ENCODER_HPP
#define OTHERLIB_SERIALIZATION_ENCODING_BINARY_ENCODER_HPP

#include <glm/gtc/type_ptr.hpp>

#include "core/defines.hpp"
#include "core/enum_formatter.hpp"
#include "serialization/reflection.hpp"

namespace other {
  namespace binary {
    namespace detail {

      template <typename T>
      inline void encode(value_type val, uint32_t size, const T& value, ostd::vector<uint8_t>& bytes) {
        const uint8_t* type_data = reinterpret_cast<const uint8_t*>(&val);
        const uint8_t* size_data = reinterpret_cast<const uint8_t*>(&size);
        const uint8_t* value_data = reinterpret_cast<const uint8_t*>(&value);
        bytes.append_range(std::span(type_data, sizeof(value_type)));
        bytes.append_range(std::span(size_data, sizeof(uint32_t)));
        bytes.append_range(std::span(value_data, *reinterpret_cast<const uint32_t*>(size_data)));
      }

      template <typename T>
      inline void encode(value_type val, const T& value, ostd::vector<uint8_t>& bytes) {
        return encode<T>(val, sizeof(T), value, bytes);
      }

      template <typename T>
      inline void encode(const T& value, ostd::vector<uint8_t>& bytes) {
        return encode<T>(get_value_type<T>(), value, bytes);
      }

      template <typename T>
        requires is_linear_algebra_type<T>
      inline void encode(const T& value, ostd::vector<uint8_t>& bytes) {
        const value_type val = get_value_type<T>();
        const uint32_t size = sizeof(T);
        const uint8_t* type_data = reinterpret_cast<const uint8_t*>(&val);
        const uint8_t* size_data = reinterpret_cast<const uint8_t*>(&size);
        const uint8_t* value_data = reinterpret_cast<const uint8_t*>(glm::value_ptr(value));
        bytes.append_range(std::span(type_data, sizeof(value_type)));
        bytes.append_range(std::span(size_data, sizeof(uint32_t)));
        bytes.append_range(std::span(value_data, sizeof(T)));
      }

      template <typename T>
        requires is_byte_buffer_type<T>
      inline void encode(const std::span<const uint8_t> buffer, ostd::vector<uint8_t>& bytes) {
        const value_type val = value_type::BYTE_BUFFER;
        const uint32_t size = static_cast<uint32_t>(buffer.size());
        const uint8_t* type_data = reinterpret_cast<const uint8_t*>(&val);
        const uint8_t* size_data = reinterpret_cast<const uint8_t*>(&size);
        const uint8_t* value_data = buffer.data();
        bytes.append_range(std::span(type_data, sizeof(value_type)));
        bytes.append_range(std::span(size_data, sizeof(uint32_t)));
        bytes.append_range(std::span(value_data, buffer.size()));
      }

      template <typename T>
        requires is_string_type<T>
      inline void encode(const T& value, ostd::vector<uint8_t>& bytes) {
        const value_type val = value_type::STRING;
        const uint32_t size = static_cast<uint32_t>(value.size());
        const uint8_t* type_data = reinterpret_cast<const uint8_t*>(&val);
        const uint8_t* size_data = reinterpret_cast<const uint8_t*>(&size);
        const uint8_t* value_data = reinterpret_cast<const uint8_t*>(value.data());
        bytes.append_range(std::span(type_data, sizeof(value_type)));
        bytes.append_range(std::span(size_data, sizeof(uint32_t)));
        bytes.append_range(std::span(value_data, value.size()));
      }

      template <typename T>
        requires(std::is_trivially_copyable_v<T> && !reflected_type<T> && !is_string_type<T> && !is_byte_buffer_type<T>)
      T decode(const std::span<const uint8_t> data, size_t& offset) {
        OTHER_ASSERT(offset + sizeof(value_type) + sizeof(uint32_t) <= data.size(), "Not enough data to read value of type {}. Expected at least {} bytes, but only {} bytes remain.",
                     typeid(T).name(), sizeof(value_type) + sizeof(uint32_t), data.size() - offset);
        value_type val = static_cast<value_type>(data[offset]);
        OTHER_ASSERT(val == get_value_type<T>(), "Type mismatch when reading value. Expected type {}, but found type {}.", std::string{ typeid(T).name() }, val);

        uint32_t size = *reinterpret_cast<const uint32_t*>(data.data() + offset + sizeof(value_type));
        OTHER_ASSERT(data.size() >= offset + sizeof(value_type) + sizeof(uint32_t) + size, "Not enough data to read value of type {}. Expected at least {} bytes, but only {} bytes remain.",
                     typeid(T).name(), sizeof(value_type) + sizeof(uint32_t) + size, data.size() - offset);

        std::span bytes = data.subspan(offset + sizeof(value_type) + sizeof(uint32_t), size);
        offset += sizeof(value_type) + sizeof(uint32_t) + size;
        return *reinterpret_cast<const T*>(bytes.data());
      }

      template <typename T>
        requires is_byte_buffer_type<T>
      T decode(const std::span<const uint8_t> data, size_t& offset) {
        value_type val = static_cast<value_type>(data[offset]);
        OTHER_ASSERT(val == value_type::BYTE_BUFFER, "Type mismatch when reading byte buffer value. Expected type BYTE_BUFFER, but found type {}.", val);

        uint32_t size = *reinterpret_cast<const uint32_t*>(data.data() + offset + sizeof(value_type));
        OTHER_ASSERT(data.size() >= offset + sizeof(value_type) + sizeof(uint32_t) + size, "Not enough data to read byte buffer value. Expected at least {} bytes, but only {} bytes remain.",
                     sizeof(value_type) + sizeof(uint32_t) + size, data.size() - offset);

        std::span<const uint8_t> bytes = data.subspan(offset + sizeof(value_type) + sizeof(uint32_t), size);
        offset += sizeof(value_type) + sizeof(uint32_t) + size;
        return T(bytes);
      }

      template <typename T>
        requires is_string_type<T>
      inline T decode(const std::span<const uint8_t> data, size_t& offset) {
        value_type val = static_cast<value_type>(data[offset]);
        OTHER_ASSERT(val == value_type::STRING, "Type mismatch when reading string value. Expected type STRING, but found type {}.", val);

        uint32_t size = *reinterpret_cast<const uint32_t*>(data.data() + offset + sizeof(value_type));
        OTHER_ASSERT(data.size() >= offset + sizeof(value_type) + sizeof(uint32_t) + size, "Not enough data to read string value. Expected at least {} bytes, but only {} bytes remain.",
                     sizeof(value_type) + sizeof(uint32_t) + size, data.size() - offset);

        std::span bytes = data.subspan(offset + sizeof(value_type) + sizeof(uint32_t), size);
        offset += sizeof(value_type) + sizeof(uint32_t) + size;
        return T(reinterpret_cast<const char*>(bytes.data()), size);
      }

    }  // namespace detail

    template <typename T>
    struct encoder {
      static_assert(false, "no detail::binary::encoder<T>: OTHER_REFLECT the type or specialize encoder<T>");
    };

    template <typename T>
      requires(std::is_trivially_copyable_v<T> && !reflected_type<T> && !is_string_type<T> && !is_byte_buffer_type<T>)
    struct encoder<T> {
      void encode(const T& value, ostd::vector<uint8_t>& bytes) {
        detail::template encode<T>(value, bytes);
      };
      opt<T> decode(const std::span<const uint8_t> data, size_t& offset) {
        if (offset + get_type_minimum_size<T>() > data.size()) {
          CORE_LOG_ERROR("Not enough data to read value of type {}. Expected at least {} bytes, but only {} bytes remain.",
                         typeid(T).name(), get_type_minimum_size<T>(), data.size() - offset);
          return std::nullopt;
        }
        return detail::template decode<T>(data, offset);
      };
    };

    template <typename T>
      requires is_linear_algebra_type<T>
    struct encoder<T> {
      void encode(const T& value, ostd::vector<uint8_t>& bytes) {
        detail::template encode<T>(value, bytes);
      };
      opt<T> decode(const std::span<const uint8_t> data, size_t& offset) {
        if (offset + get_type_minimum_size<T>() > data.size()) {
          CORE_LOG_ERROR("Not enough data to read linear algebra type {}. Expected at least {} bytes, but only {} bytes remain.", typeid(T).name(), get_type_minimum_size<T>(), data.size() - offset);
          return std::nullopt;
        }

        const size_t prefix_size = sizeof(value_type) + sizeof(uint32_t);
        value_type val = static_cast<value_type>(data[offset]);
        if (val != get_value_type<T>()) {
          CORE_LOG_ERROR("Type mismatch when reading linear algebra value. Expected type {}, but found type {}.", std::string{ typeid(T).name() }, val);
          return std::nullopt;
        }

        uint32_t size = *reinterpret_cast<const uint32_t*>(data.data() + offset + sizeof(value_type));
        if (size != sizeof(T)) {
          CORE_LOG_ERROR("Size mismatch when reading linear algebra value of type {}. Expected size {}, but found size {}.", typeid(T).name(), sizeof(T), size);
          return std::nullopt;
        }

        offset += prefix_size;

        T res;
        std::memcpy(glm::value_ptr(res), data.data() + offset, sizeof(T));

        offset += size;
        return res;
      };
    };

    template <>
    struct encoder<std::string> {
      void encode(const std::string& value, ostd::vector<uint8_t>& bytes) {
        OTHER_ASSERT(value.size() <= std::numeric_limits<uint32_t>::max(), "String field too large to serialize.");
        detail::template encode<std::string>(value, bytes);
      };

      opt<std::string> decode(const std::span<const uint8_t> data, size_t& offset) {
        if (offset + sizeof(value_type) + sizeof(uint32_t) > data.size()) {
          CORE_LOG_ERROR("Not enough data to read string value. Expected at least {} bytes, but only {} bytes remain.",
                         sizeof(value_type) + sizeof(uint32_t), data.size() - offset);
          return std::nullopt;
        }

        value_type val = static_cast<value_type>(data[offset]);
        if (val != value_type::STRING) {
          CORE_LOG_ERROR("Type mismatch when reading string value. Expected type STRING, but found type {}.", val);
          return std::nullopt;
        }

        uint32_t size = *reinterpret_cast<const uint32_t*>(data.data() + offset + sizeof(value_type));
        if (offset + sizeof(value_type) + sizeof(uint32_t) + size > data.size()) {
          CORE_LOG_ERROR("Not enough data to read string value. Expected at least {} bytes, but only {} bytes remain.",
                         sizeof(value_type) + sizeof(uint32_t) + size, data.size() - offset);
          return std::nullopt;
        }
        return detail::template decode<std::string>(data, offset);
      };
    };

    template <typename T>
      requires is_byte_buffer_type<T>
    struct encoder<T> {
      void encode(const T& value, ostd::vector<uint8_t>& bytes) {
        const std::span<const uint8_t> buffer = value;
        OTHER_ASSERT(buffer.size() <= std::numeric_limits<uint32_t>::max(), "Byte buffer field too large to serialize.");
        detail::template encode<std::span<const uint8_t>>(buffer, bytes);
      };
      opt<T> decode(const std::span<const uint8_t> data, size_t& offset) {
        return detail::template decode<T>(data, offset);
      };
    };

  }  // namespace binary
}  // namespace other

#endif  // OTHERLIB_SERIALIZATION_ENCODING_BINARY_ENCODER_HPP
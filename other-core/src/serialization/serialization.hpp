/**
 * \file serialization/serialization.hpp
 **/
#ifndef OTHER_CORE_SERIALIZATION_SERIALIZATION_HPP
#define OTHER_CORE_SERIALIZATION_SERIALIZATION_HPP

#include <span>

#include "core/defines.hpp"
#include "serialization/reflection.hpp"

namespace other {
  namespace serialization {

    std::vector<uint8_t> read_file_to_bytes(const filepath& file_path);

    template <typename T>
      requires(!std::same_as<T, std::string>)
    static inline void write_value(T value, std::vector<uint8_t>& out_bytes) {
      const uint8_t* value_bytes = reinterpret_cast<const uint8_t*>(&value);
      out_bytes.append_range(std::span(value_bytes, sizeof(T)));
    }

    void write_string_value(const std::string& str, std::vector<uint8_t>& out_bytes);

    template <typename T>
    static inline void write_list_with_2B_count(const std::vector<T>& values, std::vector<uint8_t>& out_bytes) {
      uint16_t num_values = (uint16_t)values.size();
      write_value(num_values, out_bytes);

      if (num_values > 0) {
        for (T v : values) {
          write_value(v, out_bytes);
        }
      }
    }

    template <typename T>
      requires reflected_type<T>
    static inline void write_reflected_object(const T& obj, std::vector<uint8_t>& out_bytes) {
      std::vector<uint8_t> data = type_data_handler<T>::as_bytes(obj);
      write_value<uint64_t>((uint64_t)data.size(), out_bytes);
      out_bytes.append_range(data);
    }

    void write_bytes(const void* data, size_t size, std::vector<uint8_t>& out_bytes);

    template <typename T>
      requires(!std::same_as<T, std::string>)
    static inline T read_value(const std::span<const uint8_t> buffer, size_t& cursor) {
      OTHER_ASSERT(buffer.size() >= cursor + sizeof(T), "Buffer too small to read value of size {} at cursor {}", sizeof(T), cursor);
      std::span<const uint8_t> value_span = buffer.subspan(cursor, sizeof(T));
      cursor += sizeof(T);
      return *reinterpret_cast<const T*>(value_span.data());
    }

    template <typename T>
      requires(!std::same_as<T, std::string>)
    static inline T peek_at_value(const std::span<const uint8_t> buffer, size_t cursor) {
      OTHER_ASSERT(buffer.size() >= cursor + sizeof(T), "Buffer too small to read value of size {} at cursor {}", sizeof(T), cursor);
      std::span<const uint8_t> value_span = buffer.subspan(cursor, sizeof(T));
      return *reinterpret_cast<const T*>(value_span.data());
    }

    std::string read_string_value(const std::span<const uint8_t> buffer, uint64_t length, size_t& cursor);
    std::span<const uint8_t> read_bytes(const std::span<const uint8_t> buffer, uint64_t length, size_t& cursor);

    template <typename T>
    static inline std::vector<T> read_list_with_2B_count(const std::span<const uint8_t> buffer, size_t& cursor) {
      uint16_t num_values = read_value<uint16_t>(buffer, cursor);
      std::vector<T> values;
      if (num_values > 0) {
        std::span<const T> list_span{ (const T*)(buffer.data() + cursor), num_values };
        cursor += num_values * sizeof(T);

        values.insert(values.end(), list_span.begin(), list_span.end());
      }
      return values;
    }

    template <typename T>
      requires reflected_type<T>
    static inline T read_reflected_object_blob(const std::span<const uint8_t> buffer, uint64_t length, size_t& cursor) {
      std::vector<uint8_t> data = { buffer.begin(), buffer.begin() + length };
      cursor += length;

      return type_data_handler<T>::from_bytes(data);
    }

    template <typename T>
      requires reflected_type<T>
    static inline T read_reflected_object(const std::span<const uint8_t> buffer, size_t& cursor) {
      T obj = {};
      uint64_t entity_data_length = read_value<uint64_t>(buffer, cursor);

      const std::span<const uint8_t> entity_data = buffer.subspan(cursor);
      obj = read_reflected_object_blob<T>(entity_data, entity_data_length, cursor);

      return obj;
    }

  }  // namespace serialization
}  // namespace other

#endif  // OTHER_CORE_SERIALIZATION_SERIALIZATION_HPP
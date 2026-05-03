/**
 * \file serialization/serialization.hpp
 **/
#ifndef OTHER_CORE_SERIALIZATION_SERIALIZATION_HPP
#define OTHER_CORE_SERIALIZATION_SERIALIZATION_HPP

#include <span>
#include <type_traits>

#include "core/logger.hpp"
#include "serialization/reflection.hpp"

namespace other {

  struct buffer_parsing_error : public std::runtime_error {
    buffer_parsing_error(const std::string& msg, std::string stack_trace = OTHER_STACKTRACE)
        : std::runtime_error(std::format("Buffer Parsing Error: [{}]\n{}", msg, stack_trace)) {}
  };

  template <typename T>
  concept is_readable_field =
    std::is_trivially_constructible_v<T> || reflected_type<T>;
  template <typename T>
  concept is_writable_field =
    std::is_trivially_copyable_v<T> || reflected_type<T>;

  template <typename T>
  constexpr bool has_reader = false;

  template <typename T>
    requires is_readable_field<T>
  static void validate_named_field_in_raw_buffer(const std::string_view field_name, std::span<const uint8_t> data) {
    if (data.size() < sizeof(T)) {
      throw buffer_parsing_error("Insufficient data to parse field '" + std::string(field_name) + "' of type " + std::string(typeid(T).name()));
    }
  }

  template <typename T>
    requires is_readable_field<T>
  static T parse_named_field_from_raw_buffer(const std::string_view field_name, std::span<const uint8_t> data) {
    validate_named_field_in_raw_buffer<T>(field_name, data);
    return *std::launder(reinterpret_cast<const T*>(data.data()));
  }

  template <typename T>
    requires is_writable_field<T>
  static void append_named_field_to_raw_buffer(const std::string_view field_name, const T& value, std::vector<uint8_t>& data) {
    const uint8_t* value_data = reinterpret_cast<const uint8_t*>(&value);
    if (value_data == nullptr) {
      throw buffer_parsing_error("Null data pointer when appending field '" + std::string(field_name) + "' of type " + std::string(typeid(T).name()));
    }

    data.append_range(std::span(value_data, sizeof(T)));
  }

}  // namespace other

#endif  // OTHER_CORE_SERIALIZATION_SERIALIZATION_HPP
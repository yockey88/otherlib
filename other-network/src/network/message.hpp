/**
 * \file network/message.hpp
 **/
#ifndef OTHER_NETWORK_NETWORK_MESSAGE_HPP
#define OTHER_NETWORK_NETWORK_MESSAGE_HPP

#include <type_traits>

#include "serialization/reflection.hpp"
#include "serialization/serialization.hpp"

namespace other {

  template <typename T>
    requires reflected_type<T>
  void validate_message_data(std::span<const uint8_t> data) {
    if (data.empty()) {
      throw buffer_parsing_error("Message data is empty");
    }
    if (data.size() < sizeof(T)) {
      throw buffer_parsing_error("Message data is too small to contain type " + std::string(typeid(T).name()));
    }
    if (std::ranges::all_of(data, [](uint8_t byte) { return byte == 0; })) {
      throw buffer_parsing_error("Message data is all zeros, likely indicating a parsing error");
    }
  }

  template <typename T>
    requires reflected_type<T>
  std::vector<uint8_t> serialize_message(const T& value) {
    std::vector<uint8_t> data;

    for_each(refl::reflect(value).members, [&](const auto member) {
      if constexpr (refl::descriptor::has_attribute<attr::serializable>(member) &&
                    !refl::descriptor::is_function(member)) {
        std::string_view name = refl::descriptor::get_attribute<attr::serializable>(member).display_name;
        using member_t = std::decay_t<decltype(member(value))>;
        const auto& field_value = member(value);

        if constexpr (reflected_type<member_t>) {
          data.append_range(serialize_message(field_value));
        }
        //
        else if constexpr (std::is_trivially_copyable_v<member_t>) {
          append_named_field_to_raw_buffer(name, field_value, data);
        }
        //
        else {
          static_assert(false, "Unsupported field type for serialization in serialize_message");
        }
      }
    });

    return data;
  }

  template <typename T>
    requires reflected_type<T>
  T deserialize_message(std::span<const uint8_t> data) {
    validate_message_data<T>(data);

    T value{};
    std::span<const uint8_t> remaining_data = data;

    for_each(refl::reflect(value).members, [&](auto member) {
      if constexpr (refl::descriptor::has_attribute<attr::serializable>(member) &&
                    !refl::descriptor::is_function(member)) {
        std::string_view name = refl::descriptor::get_attribute<attr::serializable>(member).display_name;
        using member_t = std::decay_t<decltype(member(value))>;

        if constexpr (reflected_type<member_t>) {
          member(value) = deserialize_message<member_t>(remaining_data);
        }
        //
        else if constexpr (std::is_default_constructible_v<member_t>) {
          member(value) = parse_named_field_from_raw_buffer<member_t>(name, remaining_data);
          remaining_data = remaining_data.subspan(sizeof(member_t));
        }
        //
        else {
          static_assert(false, "Unsupported field type for deserialization in deserialize_message");
        }
      }
    });

    return value;
  }

}  // namespace other

#endif  // OTHER_NETWORK_NETWORK_MESSAGE_HPP
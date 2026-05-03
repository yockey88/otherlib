/**
 * \file network/message.hpp
 **/
#ifndef OTHER_NETWORK_NETWORK_MESSAGE_HPP
#define OTHER_NETWORK_NETWORK_MESSAGE_HPP

#include "serialization/reflection.hpp"
#include "serialization/serialization.hpp"

namespace other {
  namespace attr {

    struct message_field : refl::attr::usage::field {
      std::string_view display_name;
      explicit constexpr message_field(const std::string_view display_name)
          : display_name(std::move(display_name)) {}
    };

  }  // namespace attr

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
      if constexpr (refl::descriptor::has_attribute<attr::message_field>(member) &&
                    !refl::descriptor::is_function(member)) {
        std::string_view name = refl::descriptor::get_attribute<attr::message_field>(member).display_name;
        const auto& field_value = member(value);
        append_named_field_to_raw_buffer(name, field_value, data);
      }
    });

    return data;
  }

  template <typename T>
    requires reflected_type<T>
  T deserialize_message(std::span<const uint8_t> data) {
    validate_message_data<T>(data);

    T value{};
    std::span remaining_data = data;

    for_each(refl::reflect(value).members, [&](auto member) {
      if constexpr (refl::descriptor::has_attribute<attr::message_field>(member) &&
                    !refl::descriptor::is_function(member)) {
        std::string_view name = refl::descriptor::get_attribute<attr::message_field>(member).display_name;
        using member_t = std::decay_t<decltype(member(value))>;
        member(value) = parse_named_field_from_raw_buffer<member_t>(name, remaining_data);
        remaining_data = remaining_data.subspan(sizeof(member_t));
      }
    });

    return value;
  }

}  // namespace other

#endif  // OTHER_NETWORK_NETWORK_MESSAGE_HPP
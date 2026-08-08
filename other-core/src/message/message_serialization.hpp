/**
 * \file message/message_serialization.hpp
 **/
#ifndef OTHER_CORE_MESSAGE_MESSAGE_SERIALIZATION_HPP
#define OTHER_CORE_MESSAGE_MESSAGE_SERIALIZATION_HPP

#include <bit>
#include <cstring>

#include "core/profiler.hpp"
#include "data-structures/std_container.hpp"
#include "serialization/reflection.hpp"
#include "serialization/serialization.hpp"

#include "message/message_fields.hpp"

/// the bus/wire codec is defined little-endian, never converted — this engine is
///  Windows-x64-only and any future big-endian target must add conversion here first
static_assert(std::endian::native == std::endian::little, "message codec requires a little-endian target");

namespace other {
  namespace attr {

    struct msg_field : public refl::attr::usage::field {
      const std::string_view name;
      const message_field field_type;
      const value_type value_type;

      constexpr msg_field() = delete;
      constexpr msg_field(message_field field_type)
          : name(kMessageFieldNames[static_cast<uint16_t>(field_type)]), field_type(field_type),
            value_type(kMessageFieldTypes[static_cast<uint16_t>(field_type)]) {}
    };

  }  // namespace attr
}  // namespace other

#define OTHER_MSG_FIELD(name, type) \
  field(name, other::attr::msg_field(other::message_field::type), other::attr::serializable(std::string_view(#name)))

namespace other {

  template <typename T>
    requires reflected_type<T>
  void validate_message_data(std::span<const uint8_t> data) {
    if (data.empty()) {
      throw buffer_parsing_error("Message data is empty");
    }
  }

  template <typename T>
    requires reflected_type<T>
  ostd::vector<uint8_t> serialize_direct(const T& value, size_t level = 0) {
    PROFILE_SECTION("serialize_direct");
    ostd::vector<uint8_t> data;

    CORE_LOG_TRACE("{}[WRITE: {}]", std::string(level * 2, ' '), get_type_name<T>());
    for_each(refl::reflect(value).members, [&](const auto member) {
      if constexpr (refl::descriptor::has_attribute<attr::msg_field>(member) &&
                    !refl::descriptor::is_function(member)) {
        std::string name = reflected_field_name(member);
        using member_t = std::decay_t<decltype(member(value))>;
        const auto& field_value = member(value);

        if constexpr (is_buffer_type<member_t>) {
          const auto& buffer = member(value);

          /// u32 length prefix: join snapshots and asset payloads routinely exceed 64 KB
          if (std::ranges::size(buffer) > std::numeric_limits<uint32_t>::max()) {
            throw buffer_parsing_error("Buffer size exceeds maximum supported size of " + std::to_string(std::numeric_limits<uint32_t>::max()));
          }

          CORE_LOG_TRACE("{}[FIELD: {}] [BLOB ({} bytes)] (type: {}, offset: {})", std::string((level + 1) * 2, ' '), name, std::ranges::size(buffer), get_value_type<T>(), data.size());
          uint32_t buff_size = static_cast<uint32_t>(std::ranges::size(buffer));
          append_named_field_to_raw_buffer(name + "_buff_len", buff_size, data);
          data.append_range(buffer);
        }
        /// nested reflected values have no formatter — they announce themselves in
        ///  the recursive call's own trace
        else if constexpr (reflected_type<member_t>) {
          CORE_LOG_TRACE("{}[FIELD: {}] [NESTED] (offset: {})", std::string((level + 1) * 2, ' '), name, data.size());
          data.append_range(serialize_direct(field_value, level + 1));
        }
        /// vectors of trivially-copyable elements ride as [u32 count][raw elements]
        ///  (transform batches and snapshot tables)
        else if constexpr (is_trivial_vector<member_t>) {
          if (field_value.size() > std::numeric_limits<uint32_t>::max()) {
            throw buffer_parsing_error("Trivial vector exceeds maximum supported element count");
          }
          CORE_LOG_TRACE("{}[FIELD: {}] [TRIVIAL VECTOR ({} elements)] (offset: {})", std::string((level + 1) * 2, ' '), name, field_value.size(), data.size());
          const uint32_t count = static_cast<uint32_t>(field_value.size());
          append_named_field_to_raw_buffer(name + "_count", count, data);
          data.append_range(std::span(reinterpret_cast<const uint8_t*>(field_value.data()), field_value.size() * sizeof(typename member_t::value_type)));
        }
        //
        else if constexpr (std::is_trivially_copyable_v<member_t>) {
          CORE_LOG_TRACE("{}[FIELD: {}] {} (type: {}, offset: {})", std::string((level + 1) * 2, ' '), name, field_value, get_value_type<member_t>(), data.size());
          append_named_field_to_raw_buffer(name, field_value, data);
        }
        //
        else {
          static_assert(false, "Unsupported field type for serialization in serialize_direct");
        }
      }
    });

    return data;
  }

  template <typename T>
    requires reflected_type<T>
  std::pair<T, size_t> deserialize_direct(std::span<const uint8_t> data, size_t level = 0) {
    PROFILE_SECTION("deserialize_direct");
    validate_message_data<T>(data);

    T value{};
    std::span<const uint8_t> remaining_data = data;

    CORE_LOG_TRACE("{}[READ: {}]", std::string(level * 2, ' '), get_type_name<T>());
    for_each(refl::reflect(value).members, [&](auto member) {
      if constexpr (refl::descriptor::has_attribute<attr::msg_field>(member) &&
                    !refl::descriptor::is_function(member)) {
        std::string name = reflected_field_name(member);
        using member_t = std::decay_t<decltype(member(value))>;
        size_t offset = data.size() - remaining_data.size();

        if constexpr (is_buffer_type<member_t>) {
          uint32_t buff_size = parse_named_field_from_raw_buffer<uint32_t>(name + "_buff_len", remaining_data);
          remaining_data = remaining_data.subspan(sizeof(uint32_t));
          if (buff_size > remaining_data.size()) {
            throw buffer_parsing_error("Buffer size specified in message data for field '" + name + "' exceeds remaining data size");
          }

          member(value) = ostd::vector<uint8_t>(remaining_data.data(), remaining_data.data() + buff_size);
          remaining_data = remaining_data.subspan(buff_size);
          CORE_LOG_TRACE("{}[FIELD: {}] [BLOB ({} bytes)] (type: {}, offset: {})", std::string((level + 1) * 2, ' '), name, buff_size, get_value_type<T>(), data.size() - remaining_data.size());
        }
        //
        else if constexpr (reflected_type<member_t>) {
          auto [deserialized_value, consumed_size] = deserialize_direct<member_t>(remaining_data, level + 1);
          member(value) = deserialized_value;
          remaining_data = remaining_data.subspan(consumed_size);
          CORE_LOG_TRACE("{}[FIELD: {}] [NESTED] (offset: {})", std::string((level + 1) * 2, ' '), name, offset);
        }
        //
        else if constexpr (is_trivial_vector<member_t>) {
          using element_t = typename member_t::value_type;
          const uint32_t count = parse_named_field_from_raw_buffer<uint32_t>(name + "_count", remaining_data);
          remaining_data = remaining_data.subspan(sizeof(uint32_t));
          const size_t byte_count = static_cast<size_t>(count) * sizeof(element_t);
          if (byte_count > remaining_data.size()) {
            throw buffer_parsing_error("Trivial vector count for field '" + name + "' exceeds remaining data size");
          }
          member(value).resize(count);
          std::memcpy(member(value).data(), remaining_data.data(), byte_count);
          remaining_data = remaining_data.subspan(byte_count);
          CORE_LOG_TRACE("{}[FIELD: {}] [TRIVIAL VECTOR ({} elements)] (offset: {})", std::string((level + 1) * 2, ' '), name, count, offset);
        }
        //
        else if constexpr (std::is_default_constructible_v<member_t>) {
          member(value) = parse_named_field_from_raw_buffer<member_t>(name, remaining_data);
          remaining_data = remaining_data.subspan(sizeof(member_t));
          CORE_LOG_TRACE("{}[FIELD: {}] {} (type: {}, offset: {})", std::string((level + 1) * 2, ' '), name, member(value), get_value_type<member_t>(), offset);
        }
        //
        else {
          static_assert(false, "Unsupported field type for deserialization in deserialize_direct");
        }
      }
    });

    return { value, data.size() - remaining_data.size() };
  }

}  // namespace other

#endif  // OTHER_CORE_MESSAGE_MESSAGE_SERIALIZATION_HPP
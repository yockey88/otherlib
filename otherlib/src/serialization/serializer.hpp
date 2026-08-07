/**
 * \file serialization/serializer.hpp
 **/
#ifndef OTHERLIB_SERIALIZATION_SERIALIZER_HPP
#define OTHERLIB_SERIALIZATION_SERIALIZER_HPP

#include "core/profiler.hpp"
#include "data-structures/std_container.hpp"
#include "serialization/encoding/binary_encoder.hpp"
#include "serialization/serialization.hpp"

namespace other {

  struct serializer {
    ostd::vector<uint8_t> data = {};

    uint8_t peek_byte() const {
      OTHER_ASSERT(!data.empty(), "Attempted to peek byte from empty serializer.");
      return data[0];
    }

    inline size_t get_data_size() const {
      return data.size();
    }

    template <typename T>
      requires(std::is_trivially_copyable_v<T> || is_linear_algebra_type<T>)
    size_t get_type_byte_size() const {
      return sizeof(value_type) + sizeof(uint32_t) + sizeof(T);
    }
    template <typename T>
      requires(is_string_type<T> || is_byte_buffer_type<T>)
    size_t get_type_byte_size(const T& value) const {
      return sizeof(value_type) + sizeof(uint32_t) + value.size();
    }
    template <typename T>
      requires reflected_type<T>
    size_t get_type_byte_size(const T& value) const {
      size_t size = sizeof(value_type) + sizeof(uint32_t);
      refl::util::for_each(refl::reflect<T>().members, [&](auto member) {
        using member_t = std::decay_t<decltype(member)>;
        if (!detail::should_serialize_member<member_t>()) {
          return;
        }

        size += this->template get_type_byte_size<member_t>(member(value));
      });
      return size;
    }

    template <typename T>
    void write_to_bytes(const T& value) {
      PROFILE_SECTION("serializer::write_to_bytes");
      other::binary::template encoder<T>{}.encode(value, data);
    }

    template <typename T>
    opt<T> read_from_bytes(size_t offset = 0) const {
      PROFILE_SECTION("serializer::read_from_bytes");
      return other::binary::template encoder<T>{}.decode(data, offset);
    }

    // void write_bytes(std::span<const uint8_t> bytes) {
    //   data.append_range(bytes);
    // }

    // void write_bytes(const uint8_t* bytes, size_t size) {
    //   data.append_range(std::span<const uint8_t>(bytes, size));
    // }

    // template <typename T>
    //   requires(std::is_trivially_copyable_v<T> && !reflected_type<T> && !is_string_type<T> && !is_byte_buffer_type<T>)
    // void write(const T& value) {
    //   value_type val = get_value_type<T>();
    //   uint32_t size = sizeof(T);

    //   const uint8_t* type_data = reinterpret_cast<const uint8_t*>(&val);
    //   const uint8_t* size_data = reinterpret_cast<const uint8_t*>(&size);
    //   const uint8_t* value_data = reinterpret_cast<const uint8_t*>(&value);

    //   data.append_range(std::span(type_data, sizeof(value_type)));
    //   data.append_range(std::span(size_data, sizeof(uint32_t)));
    //   data.append_range(std::span(value_data, sizeof(T)));
    // }

    // template <typename T>
    //   requires is_linear_algebra_type<T>
    // void write(const T& value) {
    //   value_type val = get_value_type<T>();
    //   uint32_t size = sizeof(T);
    //   const uint8_t* type_data = reinterpret_cast<const uint8_t*>(&val);
    //   const uint8_t* size_data = reinterpret_cast<const uint8_t*>(&size);
    //   const uint8_t* value_data = reinterpret_cast<const uint8_t*>(glm::value_ptr(value));

    //   data.append_range(std::span(type_data, sizeof(value_type)));
    //   data.append_range(std::span(size_data, sizeof(uint32_t)));
    //   data.append_range(std::span(value_data, sizeof(T)));
    // }

    // template <typename T>
    //   requires is_string_type<T>
    // void write(const T& value) {
    //   const std::string& str = value;
    //   OTHER_ASSERT(str.size() <= std::numeric_limits<uint32_t>::max(), "String field too large to serialize.");

    //   value_type val = value_type::STRING;
    //   uint32_t size = static_cast<uint32_t>(str.size());
    //   const uint8_t* type_data = reinterpret_cast<const uint8_t*>(&val);
    //   const uint8_t* size_data = reinterpret_cast<const uint8_t*>(&size);
    //   const uint8_t* value_data = reinterpret_cast<const uint8_t*>(str.data());

    //   data.append_range(std::span(type_data, sizeof(value_type)));
    //   data.append_range(std::span(size_data, sizeof(uint32_t)));
    //   data.append_range(std::span(value_data, str.size()));
    // }

    // template <typename T>
    //   requires((reflected_type<T> && !is_linear_algebra_type<T>) || is_string_type<T> || is_byte_buffer_type<T>)
    // void write(const T& value) {
    //   refl::util::for_each(refl::reflect<T>().members, [&](auto member) {
    //     using member_t = std::decay_t<decltype(member)>;
    //     if (!detail::should_serialize_member<member_t>()) {
    //       return;
    //     }

    //     value_type val = get_value_type<member_t>();
    //     if constexpr (reflected_type<member_t> && !is_linear_algebra_type<member_t>) {
    //       val = value_type::USER_TYPE;
    //     }
    //     write<uint8_t>(static_cast<uint8_t>(val));

    //     if constexpr (is_linear_algebra_type<member_t>) {
    //       auto value_ptr = glm::value_ptr(member(value));
    //       write<uint32_t>(sizeof(member_t));
    //       write_bytes(reinterpret_cast<const uint8_t*>(value_ptr), sizeof(member_t));
    //     } else if constexpr (reflected_type<member_t>) {
    //       write(member(value));
    //     } else if constexpr (is_string_type<member_t>) {
    //       const std::string& str = member(value);
    //       OTHER_ASSERT(str.size() <= std::numeric_limits<uint32_t>::max(), "String field too large to serialize.");
    //       write<uint32_t>(static_cast<uint32_t>(str.size()));
    //       write_bytes(std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(str.data()), str.size()));
    //     } else if constexpr (is_byte_buffer_type<member_t>) {
    //       using element_t = typename member_t::value_type;
    //       const member_t& elements = member(value);
    //       const size_t raw_size = elements.size() * sizeof(element_t);
    //       OTHER_ASSERT(raw_size + sizeof(uint32_t) <= std::numeric_limits<uint32_t>::max(), "Vector field too large to serialize.");
    //       write<uint32_t>(static_cast<uint32_t>(sizeof(uint32_t) + raw_size));
    //       write<uint32_t>(static_cast<uint32_t>(elements.size()));
    //       write_bytes(std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(elements.data()), raw_size));
    //     } else if constexpr (std::is_trivially_copyable_v<member_t>) {
    //       write<uint32_t>(sizeof(member_t));
    //       write(member(value));
    //     } else {
    //       static_assert(dependent_false<member_t>, "unsupported member type for tagged serialization");
    //     }
    //   });
    // }
  };

  // struct byte_reader {
  //   std::span<const uint8_t> data;
  //   size_t offset = 0;

  //   template <typename T>
  //   inline natural_t get_type_minimum_size() const {
  //     // type tag, 4 byte size, and the value itself
  //     return sizeof(value_type) + sizeof(uint32_t) + sizeof(T);
  //   }

  //   inline value_type peek_current_type() const {
  //     if (offset + sizeof(value_type) > data.size()) {
  //       OTHER_ASSERT(false, "Attempted to read past the end of the byte stream.");
  //     }
  //     return static_cast<value_type>(data[offset]);
  //   }

  //   template <typename T>
  //     requires(std::is_trivially_copyable_v<T> && !reflected_type<T>)
  //   opt<T> read() {
  //     if (!offset + get_type_minimum_size<T>() <= data.size()) {
  //       CORE_LOG_ERROR("Not enough data to read type {}. Expected at least {} bytes, but only {} bytes remain.", typeid(T).name(), get_type_minimum_size<T>(), data.size() - offset);
  //       return std::nullopt;
  //     }

  //     const size_t prefix_size = sizeof(value_type) + sizeof(uint32_t);
  //     value_type val = static_cast<value_type>(data[offset]);
  //     if (val != get_value_type<T>()) {
  //       CORE_LOG_ERROR("Type mismatch when reading value. Expected type {}, but found type {}.", typeid(T).name(), val);
  //       return std::nullopt;
  //     }

  //     uint32_t size = *reinterpret_cast<const uint32_t*>(data.data() + offset + sizeof(value_type));
  //     if (size != sizeof(T)) {
  //       CORE_LOG_ERROR("Size mismatch when reading value of type {}. Expected size {}, but found size {}.", typeid(T).name(), sizeof(T), size);
  //       return std::nullopt;
  //     }

  //     std::span bytes = data.subspan(offset + prefix_size, size);
  //     offset += prefix_size + size;

  //     return *reinterpret_cast<const T*>(bytes.data());
  //   }

  //   template <typename T>
  //     requires is_linear_algebra_type<T>
  //   opt<T> read() {
  //     if (!offset + get_type_minimum_size<T>() <= data.size()) {
  //       CORE_LOG_ERROR("Not enough data to read linear algebra type {}. Expected at least {} bytes, but only {} bytes remain.", typeid(T).name(), get_type_minimum_size<T>(), data.size() - offset);
  //       return std::nullopt;
  //     }

  //     const size_t prefix_size = sizeof(value_type) + sizeof(uint32_t);
  //     value_type val = static_cast<value_type>(data[offset]);
  //     if (val != get_value_type<T>()) {
  //       CORE_LOG_ERROR("Type mismatch when reading linear algebra value. Expected type {}, but found type {}.", typeid(T).name(), val);
  //       return std::nullopt;
  //     }

  //     uint32_t size = *reinterpret_cast<const uint32_t*>(data.data() + offset + sizeof(value_type));
  //     if (size != sizeof(T)) {
  //       CORE_LOG_ERROR("Size mismatch when reading linear algebra value of type {}. Expected size {}, but found size {}.", typeid(T).name(), sizeof(T), size);
  //       return std::nullopt;
  //     }

  //     std::span bytes = data.subspan(offset + prefix_size, size);
  //     offset += prefix_size + size;

  //     T res;
  //     std::memcpy(glm::value_ptr(res), bytes.data(), sizeof(T));
  //     return res;
  //   }

  //   template <typename T>
  //     requires is_string_type<T>
  //   opt<T> read() {
  //     if (offset + sizeof(value_type) + sizeof(uint32_t) > data.size()) {
  //       CORE_LOG_ERROR("Not enough data to read string type {}. Expected at least {} bytes, but only {} bytes remain.", typeid(T).name(), sizeof(value_type) + sizeof(uint32_t), data.size() - offset);
  //       return std::nullopt;
  //     }

  //     const size_t prefix_size = sizeof(value_type) + sizeof(uint32_t);
  //     value_type val = static_cast<value_type>(data[offset]);
  //     if (val != value_type::STRING) {
  //       CORE_LOG_ERROR("Type mismatch when reading string value. Expected type STRING, but found type {}.", val);
  //       return std::nullopt;
  //     }

  //     uint32_t size = *reinterpret_cast<const uint32_t*>(data.data() + offset + sizeof(value_type));
  //     if (offset + prefix_size + size > data.size()) {
  //       CORE_LOG_ERROR("Not enough data to read string value of type {}. Expected {} bytes for the string data, but only {} bytes remain.", typeid(T).name(), size, data.size() - offset - prefix_size);
  //       return std::nullopt;
  //     }

  //     std::span bytes = data.subspan(offset + prefix_size, size);
  //     offset += prefix_size + size;

  //     return T(reinterpret_cast<const char*>(bytes.data()), size);
  //   }

  //   template <typename T>
  //     requires is_byte_buffer_type<T>
  //   T read() {
  //     if (offset + sizeof(value_type) + 2 * sizeof(uint32_t) > data.size()) {
  //       CORE_LOG_ERROR("Not enough data to read byte buffer type {}. Expected at least {} bytes, but only {} bytes remain.", typeid(T).name(), sizeof(value_type) + 2 * sizeof(uint32_t), data.size() - offset);
  //       return {};
  //     }

  //     const size_t prefix_size = sizeof(value_type) + 2 * sizeof(uint32_t);
  //     value_type val = static_cast<value_type>(data[offset]);
  //     if (val != value_type::BYTE_BUFFER) {
  //       CORE_LOG_ERROR("Type mismatch when reading byte buffer value. Expected type BYTE_BUFFER, but found type {}.", val);
  //       return {};
  //     }

  //     uint32_t total_size = *reinterpret_cast<const uint32_t*>(data.data() + offset + sizeof(value_type));
  //     uint32_t element_count = *reinterpret_cast<const uint32_t*>(data.data() + offset + sizeof(value_type) + sizeof(uint32_t));
  //     size_t element_size = (total_size - sizeof(uint32_t)) / element_count;

  //     if (offset + prefix_size + total_size > data.size()) {
  //       CORE_LOG_ERROR("Not enough data to read byte buffer value of type {}. Expected {} bytes for the byte buffer data, but only {} bytes remain.", typeid(T).name(), total_size, data.size() - offset - prefix_size);
  //       return {};
  //     }

  //     std::span bytes = data.subspan(offset + prefix_size, total_size - sizeof(uint32_t));
  //     using element_t = typename T::value_type;
  //     T container(element_count);
  //     for (size_t i = 0; i < element_count; ++i) {
  //       const uint8_t* element_data = bytes.data() + i * element_size;
  //       container[i] = *reinterpret_cast<const element_t*>(element_data);
  //     }
  //     offset += prefix_size + total_size;

  //     return container;
  //   }

  //   template <typename T>
  //     requires(reflected_type<T> && !is_linear_algebra_type<T> && !is_string_type<T> && !is_byte_buffer_type<T>)
  //   opt<T> read() {
  //     if (offset + sizeof(uint8_t) > data.size()) {
  //       OTHER_ASSERT(false, "Attempted to read past the end of the byte stream.");
  //     }
  //     value_type val = static_cast<value_type>(data[offset]);
  //     if (val != value_type::USER_TYPE) {
  //       CORE_LOG_ERROR("Type mismatch when reading reflected type {}. Expected type USER_TYPE, but found type {}.", typeid(T).name(), val);
  //       return std::nullopt;
  //     }

  //     T obj{};
  //     refl::util::for_each(refl::reflect<T>().members, [&](auto member) {
  //       using member_t = std::decay_t<decltype(member)>;
  //       if (!detail::should_serialize_member<member_t>()) {
  //         return;
  //       }

  //       if (offset + sizeof(uint8_t) > data.size()) {
  //         OTHER_ASSERT(false, "Attempted to read past the end of the byte stream while reading member '{}'.", member.name);
  //       }
  //       value_type member_val = static_cast<value_type>(data[offset]);
  //       if constexpr (reflected_type<member_t>) {
  //         if (member_val != value_type::USER_TYPE) {
  //           CORE_LOG_ERROR("Type mismatch when reading reflected member '{}.{}'. Expected type USER_TYPE, but found type {}.", typeid(T).name(), member.name, member_val);
  //           return;
  //         }
  //         auto opt_member_value = read<member_t>();
  //         if (!opt_member_value.has_value()) {
  //           CORE_LOG_ERROR("Failed to read reflected member '{}.{}'.", typeid(T).name(), member.name);
  //           return;
  //         }
  //         *reinterpret_cast<member_t*>(member(obj)) = std::move(opt_member_value.value());
  //       } else if constexpr (is_string_type<member_t>) {
  //         if (member_val != value_type::STRING) {
  //           CORE_LOG_ERROR("Type mismatch when reading string member '{}.{}'. Expected type STRING, but found type {}.", typeid(T).name(), member.name, member_val);
  //           return;
  //         }
  //         uint32_t size = *reinterpret_cast<const uint32_t*>(data.data() + offset + sizeof(value_type));
  //         std::span bytes = data.subspan(offset + sizeof(value_type) + sizeof(uint32_t), size);
  //         *reinterpret_cast<member_t*>(member(obj)) = std::string(reinterpret_cast<const char*>(bytes.data()), size);
  //         offset += sizeof(value_type) + sizeof(uint32_t) + size;
  //       } else if constexpr (is_byte_buffer_type<member_t>) {
  //         if (member_val != value_type::BYTE_BUFFER) {
  //           CORE_LOG_ERROR("Type mismatch when reading byte buffer member '{}.{}'. Expected type BYTE_BUFFER, but found type {}.", typeid(T).name(), member.name, member_val);
  //           return;
  //         }
  //         uint32_t total_size = *reinterpret_cast<const uint32_t*>(data.data() + offset + sizeof(value_type));
  //         uint32_t element_count = *reinterpret_cast<const uint32_t*>(data.data() + offset + sizeof(value_type) + sizeof(uint32_t));
  //         size_t element_size = (total_size - sizeof(uint32_t)) / element_count;
  //         std::span bytes = data.subspan(offset + sizeof(value_type) + 2 * sizeof(uint32_t), total_size - sizeof(uint32_t));
  //         using element_t = typename member_t::value_type;
  //         member_t& container = *reinterpret_cast<member_t*>(member(obj));
  //         container.resize(element_count);
  //         for (size_t i = 0; i < element_count; ++i) {
  //           const uint8_t* element_data = bytes.data() + i * element_size;
  //           container[i] = *reinterpret_cast<const element_t*>(element_data);
  //         }
  //         offset += sizeof(value_type) + sizeof(uint32_t) + total_size;
  //       } else if constexpr (std::is_trivially_copyable_v<member_t>) {
  //         if (member_val != get_value_type<member_t>()) {
  //           CORE_LOG_ERROR("Type mismatch when reading member '{}.{}'. Expected type {}, but found type {}.", typeid(T).name(), member.name, typeid(member_t).name(), member_val);
  //           return;
  //         }
  //         uint32_t size = *reinterpret_cast<const uint32_t*>(data.data() + offset + sizeof(value_type));
  //         if (size != sizeof(member_t)) {
  //           CORE_LOG_ERROR("Size mismatch when reading member '{}.{}'. Expected size {}, but found size {}.", typeid(T).name(), member.name, sizeof(member_t), size);
  //           return;
  //         }
  //         std::span bytes = data.subspan(offset + sizeof(value_type) + sizeof(uint32_t), size);
  //         *reinterpret_cast<member_t*>(member(obj)) = *reinterpret_cast<const member_t*>(bytes.data());
  //         offset += sizeof(value_type) + sizeof(uint32_t) + size;
  //       } else {
  //         CORE_LOG_ERROR("Unsupported member type '{}' for member '{}.{}'.", typeid(member_t).name(), typeid(T).name(), member.name);
  //         return;
  //       }
  //     });
  //   }
  // };

}  // namespace other

#endif  // OTHERLIB_SERIALIZATION_WRITER_HPP
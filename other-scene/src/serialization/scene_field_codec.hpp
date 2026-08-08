/**
 * \file serialization/scene_field_codec.hpp
 *
 * the field stream inside a component payload:
 *
 *   field  := [field_id u64 = FNV(member name)] [tag u8 = value_type] [size u32] [bytes]
 *   scalar :  tag = get_value_type<T>()   bytes = raw value (glm via value_ptr, enums as underlying)
 *   string :  tag = STRING                bytes = chars
 *   struct :  tag = USER_TYPE             bytes = nested field stream (reflected member)
 *   vector :  tag = USER_TYPE             bytes = [count u32] then per element [tag][size][bytes]
 *
 * every field is skippable from [tag][size] alone (schema-safe: unknown ids skip, missing fields
 * default); decode never asserts on data — malformed input reports via bool/warning, not OTHER_ASSERT
 */
#ifndef OTHER_SCENE_SERIALIZATION_SCENE_FIELD_CODEC_HPP
#define OTHER_SCENE_SERIALIZATION_SCENE_FIELD_CODEC_HPP

#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <refl/refl.hpp>

#include "core/defines.hpp"
#include "core/fnv.hpp"
#include "serialization/reflection.hpp"
#include "serialization/serialization.hpp"

namespace other {
  namespace serialization {
    namespace field_codec {

      /// -- raw little-endian primitives -------------------------------------

      template <typename T>
        requires std::is_trivially_copyable_v<T>
      inline void write_raw(const T& v, ostd::vector<uint8_t>& out) {
        const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&v);
        out.insert(out.end(), bytes, bytes + sizeof(T));
      }

      template <typename T>
        requires std::is_trivially_copyable_v<T>
      inline bool read_raw(std::span<const uint8_t> data, size_t& offset, T& out) {
        if (offset + sizeof(T) > data.size()) {
          return false;
        }
        std::memcpy(&out, data.data() + offset, sizeof(T));
        offset += sizeof(T);
        return true;
      }

      inline void write_sized_string(const std::string& v, ostd::vector<uint8_t>& out) {
        OTHER_ASSERT(v.size() <= std::numeric_limits<uint32_t>::max(), "string too large to serialize");
        write_raw<uint32_t>(static_cast<uint32_t>(v.size()), out);
        out.insert(out.end(), reinterpret_cast<const uint8_t*>(v.data()), reinterpret_cast<const uint8_t*>(v.data()) + v.size());
      }

      inline bool read_sized_string(std::span<const uint8_t> data, size_t& offset, std::string& out) {
        uint32_t length = 0;
        if (!read_raw(data, offset, length) || offset + length > data.size()) {
          return false;
        }
        if (length == 0) {
          out.clear();
        } else {
          out.assign(reinterpret_cast<const char*>(data.data() + offset), length);
        }
        offset += length;
        return true;
      }

      /// -- value classification ----------------------------------------------

      template <typename T>
      concept scene_scalar_value =
        (std::is_arithmetic_v<T> || std::is_enum_v<T>) && !std::is_pointer_v<T>;

      template <typename T>
      concept scene_glm_value = is_linear_algebra_type<T> || std::is_same_v<T, glm::quat>;

      template <typename T>
      concept scene_reflected_value =
        !scene_scalar_value<T> && !scene_glm_value<T> && !is_stringlike_type<T> &&
        refl::is_reflectable<T>() && std::default_initializable<T>;

      template <typename T>
      concept scene_container_value =
        is_container_type<T> && !is_stringlike_type<T> && !scene_glm_value<T> &&
        requires(T& c, typename T::value_type v) { c.push_back(std::move(v)); };

      template <typename T>
      concept scene_encodable_value =
        scene_scalar_value<T> || scene_glm_value<T> || is_stringlike_type<T> ||
        scene_reflected_value<T> || scene_container_value<T>;

      template <typename T>
      consteval value_type tag_of() {
        if constexpr (std::is_enum_v<T>) {
          return get_value_type<std::underlying_type_t<T>>();
        } else if constexpr (scene_scalar_value<T> || scene_glm_value<T>) {
          return get_value_type<T>();
        } else if constexpr (is_stringlike_type<T>) {
          return value_type::STRING;
        } else {
          return value_type::USER_TYPE;
        }
      }

      /// -- tagged values: [tag u8][size u32][bytes] --------------------------

      template <typename T>
        requires scene_encodable_value<std::remove_cvref_t<T>>
      void encode_value(const T& v, ostd::vector<uint8_t>& out);

      template <typename T>
        requires scene_encodable_value<T>
      bool decode_value(std::span<const uint8_t> data, size_t& offset, T& out);

      /// reads [tag][size] and yields the body span; false = truncated stream. on success the offset
      /// consumes the whole frame, keeping the stream aligned even if the caller rejects the body (type mismatch)
      inline bool read_frame(std::span<const uint8_t> data, size_t& offset, uint8_t& tag, std::span<const uint8_t>& body) {
        uint8_t frame_tag = 0;
        uint32_t size = 0;
        if (!read_raw(data, offset, frame_tag) || !read_raw(data, offset, size)) {
          return false;
        }
        if (offset + size > data.size()) {
          return false;
        }
        tag = frame_tag;
        body = data.subspan(offset, size);
        offset += size;
        return true;
      }

      /// skip one [tag][size][bytes] frame; false if truncated
      inline bool skip_value(std::span<const uint8_t> data, size_t& offset) {
        uint8_t tag = 0;
        std::span<const uint8_t> body = {};
        return read_frame(data, offset, tag, body);
      }

      template <typename T>
        requires scene_encodable_value<T>
      bool decode_body(uint8_t tag, std::span<const uint8_t> body, T& out);

      /// -- reflected structs: field streams ---------------------------------

      template <typename T>
        requires scene_reflected_value<std::remove_cvref_t<T>>
      void encode_reflected(const T& value, ostd::vector<uint8_t>& out) {
        refl::util::for_each(refl::reflect<std::remove_cvref_t<T>>().members, [&](auto member) {
          using member_descriptor_t = std::decay_t<decltype(member)>;
          if constexpr (other::detail::should_serialize_member<member_descriptor_t>()) {
            write_raw<natural_t>(FNV(std::string{ member.name }), out);
            encode_value(member(value), out);
          }
        });
      }

      /// decode fields over @p value in place; unknown ids skip forward, body mismatches
      /// warn and continue (the frame is already consumed); truncation fails the decode
      template <typename T>
        requires scene_reflected_value<std::remove_cvref_t<T>>
      bool decode_reflected(std::span<const uint8_t> data, T& value, ostd::vector<std::string>* warnings) {
        size_t offset = 0;
        while (offset < data.size()) {
          natural_t field_id = 0;
          if (!read_raw(data, offset, field_id)) {
            return false;
          }

          uint8_t tag = 0;
          std::span<const uint8_t> body = {};
          if (!read_frame(data, offset, tag, body)) {
            return false;
          }

          bool matched = false;
          bool decoded = true;
          refl::util::for_each(refl::reflect<std::remove_cvref_t<T>>().members, [&](auto member) {
            using member_descriptor_t = std::decay_t<decltype(member)>;
            if constexpr (other::detail::should_serialize_member<member_descriptor_t>()) {
              if (matched || FNV(std::string{ member.name }) != field_id) {
                return;
              }
              matched = true;
              decoded = decode_body(tag, body, member(value));
            }
          });

          if (matched && !decoded && warnings != nullptr) {
            warnings->push_back(std::format("field {:#018x} of '{}' had an unexpected encoding and was skipped", field_id, get_type_name_safe<std::remove_cvref_t<T>>()));
          }
        }
        return true;
      }

      /// -- implementations ---------------------------------------------------

      template <typename T>
        requires scene_encodable_value<std::remove_cvref_t<T>>
      void encode_value(const T& v, ostd::vector<uint8_t>& out) {
        using no_cvref_t = std::remove_cvref_t<T>;
        write_raw<uint8_t>(static_cast<uint8_t>(tag_of<no_cvref_t>()), out);

        if constexpr (scene_scalar_value<no_cvref_t>) {
          write_raw<uint32_t>(sizeof(no_cvref_t), out);
          write_raw(v, out);
        } else if constexpr (scene_glm_value<no_cvref_t>) {
          write_raw<uint32_t>(sizeof(no_cvref_t), out);
          const uint8_t* bytes = reinterpret_cast<const uint8_t*>(glm::value_ptr(v));
          out.insert(out.end(), bytes, bytes + sizeof(no_cvref_t));
        } else if constexpr (is_stringlike_type<no_cvref_t>) {
          const std::string text{ v };
          OTHER_ASSERT(text.size() <= std::numeric_limits<uint32_t>::max(), "string too large to serialize");
          write_raw<uint32_t>(static_cast<uint32_t>(text.size()), out);
          out.insert(out.end(), reinterpret_cast<const uint8_t*>(text.data()), reinterpret_cast<const uint8_t*>(text.data()) + text.size());
        } else if constexpr (scene_container_value<no_cvref_t>) {
          ostd::vector<uint8_t> body = {};
          write_raw<uint32_t>(static_cast<uint32_t>(std::ranges::size(v)), body);
          for (const auto& element : v) {
            encode_value(element, body);
          }
          write_raw<uint32_t>(static_cast<uint32_t>(body.size()), out);
          out.insert(out.end(), body.begin(), body.end());
        } else {
          static_assert(scene_reflected_value<no_cvref_t>);
          ostd::vector<uint8_t> body = {};
          encode_reflected(v, body);
          write_raw<uint32_t>(static_cast<uint32_t>(body.size()), out);
          out.insert(out.end(), body.begin(), body.end());
        }
      }

      template <typename T>
        requires scene_encodable_value<T>
      bool decode_body(uint8_t tag, std::span<const uint8_t> body, T& out) {
        if (static_cast<value_type>(tag) != tag_of<T>()) {
          return false;
        }

        if constexpr (scene_scalar_value<T>) {
          if (body.size() != sizeof(T)) {
            return false;
          }
          std::memcpy(&out, body.data(), sizeof(T));
          return true;
        } else if constexpr (scene_glm_value<T>) {
          if (body.size() != sizeof(T)) {
            return false;
          }
          std::memcpy(glm::value_ptr(out), body.data(), sizeof(T));
          return true;
        } else if constexpr (is_stringlike_type<T>) {
          if (body.empty()) {
            out.clear();
          } else {
            out.assign(reinterpret_cast<const char*>(body.data()), body.size());
          }
          return true;
        } else if constexpr (scene_container_value<T>) {
          size_t body_offset = 0;
          uint32_t count = 0;
          if (!read_raw(body, body_offset, count)) {
            return false;
          }
          out.clear();
          for (uint32_t i = 0; i < count; ++i) {
            typename T::value_type element{};
            if (!decode_value(body, body_offset, element)) {
              return false;
            }
            out.push_back(std::move(element));
          }
          return true;
        } else {
          static_assert(scene_reflected_value<T>);
          return decode_reflected(body, out, nullptr);
        }
      }

      template <typename T>
        requires scene_encodable_value<T>
      bool decode_value(std::span<const uint8_t> data, size_t& offset, T& out) {
        uint8_t tag = 0;
        std::span<const uint8_t> body = {};
        if (!read_frame(data, offset, tag, body)) {
          return false;
        }
        return decode_body(tag, body, out);
      }

    }  // namespace field_codec
  }  // namespace serialization
}  // namespace other

#endif  // OTHER_SCENE_SERIALIZATION_SCENE_FIELD_CODEC_HPP

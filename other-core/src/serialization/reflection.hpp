/**
 * \file serialization/reflection.hpp
 **/
#ifndef OTHER_CORE_REFLECTION_HPP
#define OTHER_CORE_REFLECTION_HPP

#include <concepts>
#include <cstdint>
#include <map>
#include <ranges>
#include <string>
#include <vector>

#include <flatbuffers/flexbuffers.h>
#include <glm/glm.hpp>
#include <magic_enum/magic_enum.hpp>
#include <refl/refl.hpp>

#include "core/defines.hpp"
#include "core/fnv.hpp"
#include "core/logger.hpp"
#include "core/subsystem.hpp"

namespace other {

  class type_database;

  namespace attr {

    struct serializable : refl::attr::usage::field, refl::attr::usage::function {};

  }  // namespace attr

  struct reflection_data {
    struct member {
      enum member_type {
        FIELD,
        FUNCTION,
      } type;

      size_t size;
      value_type value_type;

      std::string name;
      opt<std::string> display_name;

      std::string get_name() const;
    };

    uint64_t type_hash;
    std::string type_name;
    std::vector<member> member_descriptors;

    std::vector<uint64_t> base_types;
  };

  template <typename T>
  struct type_data_handler;

  template <typename T>
  concept is_linear_algebra_type =
    std::is_same_v<T, glm::vec2> ||
    std::is_same_v<T, glm::vec3> ||
    std::is_same_v<T, glm::vec4> ||
    std::is_same_v<T, glm::dvec2> ||
    std::is_same_v<T, glm::dvec3> ||
    std::is_same_v<T, glm::dvec4> ||
    std::is_same_v<T, glm::mat2> ||
    std::is_same_v<T, glm::mat3> ||
    std::is_same_v<T, glm::mat4>;

  template <typename T>
  concept has_iterators = requires(const T& value) {
    { std::ranges::begin(value) } -> std::same_as<typename T::const_iterator>;
    { std::ranges::end(value) } -> std::same_as<typename T::const_iterator>;
  };

  template <typename T>
  concept has_reverse_iterators = requires(const T& value) {
    { std::ranges::rbegin(value) } -> std::same_as<typename T::const_reverse_iterator>;
    { std::ranges::rend(value) } -> std::same_as<typename T::const_reverse_iterator>;
  };

  template <typename T>
  concept is_iterable_type = has_iterators<T> && has_reverse_iterators<T>;

  template <typename T>
  concept is_container =
    is_iterable_type<T> &&
    requires(const T& value) {
      { std::ranges::size(value) } -> std::convertible_to<std::size_t>;
      typename T::value_type;
    };

  template <typename T>
  concept streamable_type = requires(std::ostream& os, const T& value) {
    { os << value } -> std::same_as<std::ostream&>;
  };

  template <typename T>
  concept is_stringlike_type =
    std::convertible_to<T, std::string> || std::convertible_to<T, std::string_view> ||
    std::convertible_to<T, const char*> ||
    std::convertible_to<T, char*>;

  template <typename T>
  concept has_begin = requires(const T& value) { { std::ranges::begin(value) } -> std::same_as<typename T::const_iterator>; };
  template <typename T>
  concept has_end = requires(const T& value) { { std::ranges::end(value) } -> std::same_as<typename T::const_iterator>; };
  template <typename T>
  concept has_size = requires(const T& value) { { std::ranges::size(value) } -> std::convertible_to<std::size_t>; };
  template <typename T>
  concept has_value_type = requires { typename T::value_type; };
  template <typename T>
  concept is_container_type = has_begin<T> && has_end<T> && has_size<T> && has_value_type<T>;

  template <typename T>
  concept has_type_data_handler =
    requires { type_data_handler<T>::get_reflection_data(std::declval<const T&>()); } &&
    requires { type_data_handler<T>::as_string(std::declval<const T&>()); } &&
    requires { type_data_handler<T>::as_string(std::declval<const std::string&>(), std::declval<const T&>()); } &&
    requires { type_data_handler<T>::as_bytes(std::declval<const T&>()); } &&
    requires { type_data_handler<T>::from_bytes(std::declval<const std::vector<uint8_t>&>()); };

  template <typename T>
  concept is_non_stringlike_container_type = is_container_type<T> && !is_stringlike_type<T>;

  template <typename T>
  concept is_streamable_type = is_stringlike_type<T> || streamable_type<T>;

  template <typename T>
  concept meets_core_reflection_requirements = std::default_initializable<T> && refl::is_reflectable<T>();

  template <typename T>
  concept reflected_type = meets_core_reflection_requirements<T> && has_type_data_handler<T>;

  struct serializer {
    struct field_writer {
      template <typename U>
      constexpr void operator()(std::ostream& os, const U& value, uint32_t indent_level, const std::string& name, bool new_line = true) const {
        /// exclude linear algebra types from this because they look gross
        if constexpr (reflected_type<U> && !is_linear_algebra_type<U>) {
          os << serializer{}.write_fields_to_string<U>(name, value, indent_level + 1);
        }
        /// string-like container special case
        else if constexpr (is_stringlike_type<U>) {
          os << std::string(indent_level * 2, ' ') << name << " = " << '"' << value << '"';
        }
        /// other container types
        else if constexpr (is_container<U> && !is_stringlike_type<U>) {
          os << std::string(indent_level * 2, ' ') << name << " = [";
          using value_t = typename U::value_type;

          size_t count = 0;
          for (const auto& val : value) {
            if constexpr (reflected_type<value_t> && !is_linear_algebra_type<value_t>) {
              os << "\n";
              os << serializer{}.write_fields_to_string<value_t>(name + "[" + std::to_string(count) + "]", val, indent_level + 1);
            } else if constexpr (is_stringlike_type<value_t>) {
              os << "[" << count++ << "] = \"" << val << "\"";
            } else if constexpr (is_container<value_t>) {
              field_writer{}(os, val, indent_level + 1, std::format("[{}]", count++), false);
            } else if constexpr (is_streamable_type<value_t>) {
              os << "[" << count++ << "] = " << val;
            } else {
              os << "[" << count++ << "] = [failed to serialize type: " << typeid(value_t).name() << "]";
            }

            if (count < std::ranges::size(value)) {
              os << ", ";
            }
          }

          os << "]";
        }
        /// primitive types
        else {
          os << std::string(indent_level * 2, ' ') << name << " = ";
          if constexpr (std::is_same_v<U, char>) {
            os << '\'' << value << '\'';
          } else if constexpr (std::is_enum_v<U>) {
            os << magic_enum::enum_name(value);
          } else if constexpr (is_streamable_type<U>) {
            os << value;
          } else {
            os << "[failed to serialize type: " << typeid(U).name() << "]";
          }
        }
        os << ";";

        if (new_line) {
          os << '\n';
        }
      }

      template <typename U>
      constexpr void operator()(flexbuffers::Builder& data, const U& value, uint32_t field_idx, const std::string& name) const {
        /// we serialize both reflected types and linear algebra types to bytes
        if constexpr (reflected_type<U> && !is_linear_algebra_type<U>) {
          std::vector<uint8_t> bytes = serializer{}.write_fields_to_bytes(value);
          data.Blob(name.c_str(), bytes);
        }
        /// we simply write the vectors in because they are simple in memory and simple to parse
        else if constexpr (is_linear_algebra_type<U>) {
          // for linear algebra types, we can use the blob type to store them
          std::vector<uint8_t> bytes(sizeof(U));
          std::memcpy(bytes.data(), &value, sizeof(U));
          data.Blob(name.c_str(), bytes);
        }
        /// strings handled naturally by default case, this is for vectors, arrays, etc... of reflected types
        else if constexpr (is_non_stringlike_container_type<U>) {
          // std::span<typename U::value_type> values{ std::ranges::begin(value), std::ranges::size(value) };

          // size_t start = data.StartVector(name.c_str());
          // for (const auto& val : values) {
          //   // std::vector<uint8_t> bytes = serializer{}.write_fields_to_bytes(val);
          //   // data.Blob(name.c_str(), bytes);
          // }
          // data.EndVector(start, true, false);

        }
        /// simple types, int float, etc...
        else {
          if constexpr (is_stringlike_type<U>) {
            data.String(name.c_str(), value);
          } else if constexpr (std::is_same_v<U, int8_t> || std::is_same_v<U, int16_t> ||
                               std::is_same_v<U, int32_t> || std::is_same_v<U, int64_t> || std::same_as<U, char>) {
            data.Int(name.c_str(), static_cast<int64_t>(value));
          } else if constexpr (std::is_same_v<U, uint8_t> || std::is_same_v<U, uint16_t> ||
                               std::is_same_v<U, uint32_t> || std::is_same_v<U, uint64_t>) {
            data.UInt(name.c_str(), static_cast<uint64_t>(value));
          } else if constexpr (std::is_same_v<U, float> || std::is_same_v<U, double> ||
                               std::is_same_v<U, long double>) {
            data.Float(name.c_str(), static_cast<float>(value));
          } else if constexpr (std::is_floating_point_v<U>) {
            data.Double(name.c_str(), static_cast<double>(value));
          } else if constexpr (std::is_same_v<U, bool>) {
            data.Bool(name.c_str(), value);
          } else {
            static_assert(!std::is_same_v<U, U>, "Unsupported type for serialization.");
          }
        }
      }
    };

    struct field_reader {
    };

    template <typename T>
      requires reflected_type<T>
    std::string write_fields_to_string(const std::string& name, const T& value, int32_t indent_level = 1) const;

    template <typename T>
      requires reflected_type<T>
    std::vector<uint8_t> write_fields_to_bytes(const T& value) const;

    template <typename T>
      requires reflected_type<T>
    T read_fields_from_bytes(const std::vector<uint8_t>& data) const;

    template <typename T>
      requires reflected_type<T>
    T read_from_file(const std::string& file_path) const;
  };

  class type_database : public subsystem<type_database> {
   public:
    type_database() = default;

    template <typename T>
      requires reflected_type<T>
    reflection_data* get_reflection_data(const T& value);

   private:
    std::map<uint64_t, reflection_data> data_map;
  };

  OTHER_SUBSYSTEM(type_database);

  template <typename T>
    requires reflected_type<T>
  std::string serializer::write_fields_to_string(const std::string& name, const T& value, int32_t indent_level) const {
    std::stringstream ss;
    std::string indent = std::string((indent_level - 1) * 2, ' ');
    ss << indent << name;
    if constexpr (std::is_enum_v<T>) {
      ss << " = " << magic_enum::enum_name(value) << ";";
      return ss.str();
    } else if constexpr (is_stringlike_type<T>) {
      ss << " = \"" << value << "\";";
      return ss.str();
    } else if constexpr (is_container<T> && !is_stringlike_type<T>) {
      ss << " = [ ";
      size_t count = 0;
      for (auto itr = std::ranges::begin(value); itr != std::ranges::end(value); ++itr) {
        field_writer{}(ss, *itr, indent_level + 1, std::format("[{}]", std::to_string(count++)), false);  // false = no new line
      }
      ss << std::string(indent_level * 2, ' ') << " ]";
      return ss.str();
    } else {
      ss << " = {\n";
      for_each(refl::reflect(value).members, [&](auto member) {
        if constexpr (refl::descriptor::has_attribute<attr::serializable>(member)) {
          std::string name = std::string{ member.name };
          opt<std::string> friendly_name = {};

          if constexpr (refl::descriptor::is_property(member)) {
            friendly_name = refl::descriptor::get_property(member).friendly_name;
          }
          name = friendly_name.value_or(name);

          field_writer{}(ss, member(value), indent_level + 1, name);
        }
      });
    }
    ss << indent << "}";
    return ss.str();
  }

  template <typename T>
    requires reflected_type<T>
  std::vector<uint8_t> serializer::write_fields_to_bytes(const T& value) const {
    reflection_data* refl_data = type_database::get()->get_reflection_data(value);
    OTHER_ASSERT(refl_data != nullptr, "Failed to get reflection data for type '{}'.", std::string{ refl::reflect(value).name });

    flexbuffers::Builder builder;
    size_t start = builder.StartMap();
    builder.UInt("type-hash", refl_data->type_hash);
    builder.String("type-name", refl_data->type_name);
    builder.UInt("num-fields", refl_data->member_descriptors.size());

    size_t idx = 0;
    for_each(refl::reflect(value).members, [&](auto member) {
      std::string name = std::string{ member.name };
      if constexpr (refl::descriptor::has_attribute<attr::serializable>(member) &&
                    !refl::descriptor::is_function(member)) {
        field_writer{}(builder, member(value), idx, name);
        ++idx;
      }
    });
    builder.EndMap(start);
    builder.Finish();

    return builder.GetBuffer();
  }

  template <typename T>
  decltype(auto) get_field(flexbuffers::Reference& ref, const std::string& field_name) {
    if constexpr (other::reflected_type<T>) {
      if constexpr (!other::is_linear_algebra_type<T>) {
        auto blob = ref.AsBlob();
        std::vector<uint8_t> bytes(blob.data(), blob.data() + blob.size());
        return other::type_data_handler<T>::from_bytes(bytes);
      } else {
        auto blob = ref.AsBlob();
        if constexpr (std::is_same_v<T, glm::vec2>) {
          return *reinterpret_cast<const glm::vec2*>(blob.data());
        } else if constexpr (std::is_same_v<T, glm::vec3>) {
          return *reinterpret_cast<const glm::vec3*>(blob.data());
        } else if constexpr (std::is_same_v<T, glm::vec4>) {
          return *reinterpret_cast<const glm::vec4*>(blob.data());
        }
      }
    }

    else if constexpr (!other::reflected_type<T>) {
      if constexpr (std::is_same_v<T, int8_t>) {
        return ref.AsInt8();
      } else if constexpr (std::is_same_v<T, int16_t>) {
        return ref.AsInt16();
      } else if constexpr (std::is_same_v<T, int32_t>) {
        return ref.AsInt32();
      } else if constexpr (std::is_same_v<T, int64_t>) {
        return ref.AsInt64();
      } else if constexpr (std::is_same_v<T, uint8_t>) {
        return ref.AsUInt8();
      } else if constexpr (std::is_same_v<T, uint16_t>) {
        return ref.AsUInt16();
      } else if constexpr (std::is_same_v<T, uint32_t>) {
        return ref.AsUInt32();
      } else if constexpr (std::is_same_v<T, uint64_t>) {
        return ref.AsUInt64();
      } else if constexpr (std::is_same_v<T, float>) {
        return ref.AsFloat();
      } else if constexpr (std::is_same_v<T, double>) {
        return ref.AsDouble();
      } else if constexpr (std::is_same_v<T, std::string>) {
        return ref.AsString().str();
      } else if constexpr (std::is_same_v<T, bool>) {
        return ref.AsBool();
      } else {
        CORE_LOG_WARN("Primitive Field '{}' has unsupported value type '{}'.", field_name, other::get_value_type<T>());
        return T{};
      }
    } else {
      CORE_LOG_WARN("Field '{}' has unsupported value type '{}'.", field_name, other::get_value_type<T>());
      return T{};
    }
  }

  template <typename T>
    requires reflected_type<T>
  T serializer::read_fields_from_bytes(const std::vector<uint8_t>& data) const {
    auto outer_map = flexbuffers::GetRoot(data).AsMap();
    CORE_LOG_INFO("Deserialized type hash: {}", outer_map["type-hash"].AsUInt64());
    CORE_LOG_INFO("Deserialized type name: {}", outer_map["type-name"].AsString().str());
    CORE_LOG_INFO("Deserialized number of fields: {}", outer_map["num-fields"].AsUInt64());
    /**
     * \todo: check num fields and type-hash against version requirements to validate version compatibility
     **/

    T deserialized_obj;
    for_each(refl::reflect(deserialized_obj).members, [&](auto member) {
      if constexpr (refl::descriptor::has_attribute<other::attr::serializable>(member) &&
                    !refl::descriptor::is_function(member)) {
        std::string name = std::string{ member.name };
        flexbuffers::Reference reference = outer_map[name.c_str()];
        if (reference.IsNull()) {
          CORE_LOG_WARN("Field '{}' not found in serialized data.", name);
          return;
        }

        using member_t = std::remove_cvref_t<decltype(member(deserialized_obj))>;
        member(deserialized_obj) = other::get_field<member_t>(reference, std::string{ member.name });
      }
    });

    return deserialized_obj;
  }

  template <typename T>
    requires reflected_type<T>
  T serializer::read_from_file(const std::string& file_path) const {
    std::vector<uint8_t> bytes;
    {
      std::ifstream ifs(file_path, std::ios::binary);
      if (!ifs.is_open()) {
        CORE_LOG_ERROR("Failed to open file '{}'.", file_path);
        return T{};
      }

      ifs.seekg(0, std::ios::end);
      size_t size = ifs.tellg();
      ifs.seekg(0, std::ios::beg);

      bytes.resize(size);
      ifs.read(reinterpret_cast<char*>(bytes.data()), size);
    }
    return read_fields_from_bytes<T>(bytes);
  }

  template <typename T>
    requires reflected_type<T>
  reflection_data* type_database::get_reflection_data(const T& value) {
    /// this works because its a template function, so it will be instantiated for each type T
    static const auto refl_data = refl::reflect(value);
    static const std::string refl_type_name = std::string{ refl_data.name };
    static const uint64_t type_hash = FNV(refl_type_name);

    {
      auto it = data_map.find(type_hash);
      if (it != data_map.end()) {
        CORE_LOG_TRACE("Reflection data for type '{}' already exists.", it->second.type_name);
        return &it->second;
      }
    }
    auto [it, inserted] = data_map.emplace(type_hash, reflection_data{});
    if (!inserted || it == data_map.end()) {
      CORE_LOG_ERROR("Failed to insert reflection data for type '{}'.", refl_type_name);
      return nullptr;
    }
    CORE_LOG_DEBUG("Inserting reflection data for type '{}'.", refl_type_name);

    it->second.type_hash = type_hash;
    it->second.type_name = refl_type_name;
    for_each(refl::reflect(value).members, [&](auto member) {
      reflection_data::member m;

      m.type = reflection_data::member::FIELD;
      m.name = std::string{ member.name };
      m.display_name = {};
      if constexpr (refl::descriptor::is_function(member)) {
        m.type = reflection_data::member::FUNCTION;
        if constexpr (refl::descriptor::is_property(member)) {
          if (opt<const char*> friendly_name = refl::descriptor::get_property(member).friendly_name; friendly_name.has_value()) {
            m.display_name = std::string{ *friendly_name };
          }
        }
      }

      using member_t = decltype(member(value));

      m.value_type = get_value_type<member_t>();
      CORE_LOG_DEBUG("Member '{}' [{}] of type '{}' has value type '{}'.", m.name, m.display_name ? *m.display_name : m.name, m.type == reflection_data::member::FIELD ? "field" : "function", m.value_type);
      if (m.value_type == value_type::USER_TYPE) {
        if constexpr (reflected_type<member_t>) {
          CORE_LOG_DEBUG("    > type reflected = {}", std::string{ refl::reflect<member_t>().name });
        }
      }
      m.size = sizeof(member_t);

      CORE_LOG_TRACE("Adding member '{}' [{}] of type '{}' to reflection data for '{}'.", m.name, m.display_name ? *m.display_name : m.name, m.type == reflection_data::member::FIELD ? "field" : "function", it->second.type_name);
      it->second.member_descriptors.push_back(m);
    });

    for_each(refl::reflect(value).bases, [&](auto base) { it->second.base_types.push_back(base.hash); });

    return &it->second;
  }

}  // namespace other

#define VA_ARGS(...) , ##__VA_ARGS__

#define OTHER_REFLECTABLE(T)         \
  friend class other::type_database; \
  friend struct other::serializer;   \
  friend struct other::type_data_handler<T>;

#define OTHER_TYPE_HANDLER(T)                                                                                                                               \
  template <>                                                                                                                                               \
  struct other::type_data_handler<T> {                                                                                                                      \
    static other::reflection_data& get_reflection_data(const T& value) { return *other::type_database::get()->get_reflection_data<T>(value); }              \
    static std::string as_string(const T& value) { return other::serializer{}.write_fields_to_string<T>(std::string{ refl::reflect(value).name }, value); } \
    static std::string as_string(const std::string& name, const T& value) { return other::serializer{}.write_fields_to_string<T>(name, value); }            \
    static std::vector<uint8_t> as_bytes(const T& value) { return other::serializer{}.write_fields_to_bytes<T>(value); }                                    \
    static T from_bytes(const std::vector<uint8_t>& data) { return other::serializer{}.read_fields_from_bytes<T>(data); }                                   \
  };                                                                                                                                                        \
  static_assert(other::reflected_type<T>, "Type '" #T "' does not meet the requirements for reflection. Ensure it is default constructible and reflectable.");

#define OTHER_REFLECT(T, ...)             \
  REFL_AUTO(type(T) VA_ARGS(__VA_ARGS__)) \
  OTHER_TYPE_HANDLER(T)

#define OTHER_REFLECT_DERIVED(T, BT, ...) \
  REFL_AUTO(T, BT VA_ARGS(__VA_ARGS__))   \
  OTHER_TYPE_HANDLER(T)

OTHER_REFLECT(
  other::value_type
)

#endif  // OTHER_CORE_REFLECTION_HPP
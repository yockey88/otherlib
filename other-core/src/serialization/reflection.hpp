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
#include <type_traits>
#include <vector>

#include <glm/glm.hpp>
#include <magic_enum/magic_enum.hpp>
#include <refl/refl.hpp>

#include "core/defines.hpp"
#include "core/logger.hpp"
#include "core/profiler.hpp"
#include "core/subsystem.hpp"
#include "data-structures/std_container.hpp"

namespace other {

  class type_database;

  using type_key = uint64_t;

  template <typename T>
  inline type_key type_key_of() {
    return typeid(T).hash_code();
  }

  namespace attr {

    struct serializable : refl::attr::usage::field {
      const std::string_view display_name;
      bool editable = true;

      constexpr serializable() = default;
      explicit constexpr serializable(bool editable) : editable(editable) {}
      explicit constexpr serializable(const std::string_view display_name, bool editable = true)
          : display_name(std::move(display_name)), editable(editable) {}
    };

    struct native_only : refl::attr::usage::field {
      constexpr native_only() = default;
    };

    struct version_tag : refl::attr::usage::field {
      uint64_t version;
      constexpr version_tag(uint64_t version) : version(version) {}
    };

    template <typename T>
      requires std::is_integral_v<T> || std::is_floating_point_v<T>
    struct clamp : refl::attr::usage::field {
      T min = std::numeric_limits<T>::min();
      T max = std::numeric_limits<T>::max();

      constexpr clamp() = default;
      constexpr clamp(int64_t min, int64_t max) : min(min), max(max) {}
    };

  }  // namespace attr

  struct reflection_data {
    enum : uint8_t {
      NONE = 0,
      SERIALIZABLE = 1 << 0,
      SCRIPT_VISIBLE = 1 << 1,
      READ_ONLY = 1 << 2,
      EDITABLE = 1 << 3,
      NATIVE_ONLY = 1 << 4,
    };

    struct member {
      struct param_desc {
        std::string name;
        other::value_type type;
      };
      enum member_type {
        FIELD,
        FUNCTION,
      } type;

      size_t size;
      size_t offset;
      value_type value_type;

      std::string name;
      opt<std::string> display_name;

      // attribute flags
      uint8_t flags = reflection_data::NONE;
      uint32_t since_version = 0;

      // functions only
      opt<other::value_type> return_type;
      ostd::vector<param_desc> parameters;

      std::string get_name() const;
    };

    uint64_t type_hash;
    std::string type_name;
    ostd::vector<member> member_descriptors;

    ostd::vector<uint64_t> base_types;
  };

  static inline type_key type_key_of(const reflection_data& rd) {
    OTHER_ASSERT(rd.type_hash != 0, "reflection_data has no type_hash");
    return rd.type_hash;
  }

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
    (std::convertible_to<T, std::string> || std::convertible_to<T, std::string_view>) ||
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
    requires { type_data_handler<T>::as_string(std::declval<const std::string&>(), std::declval<const T&>()); };

  template <typename T>
  concept is_non_stringlike_container_type = is_container_type<T> && !is_stringlike_type<T>;

  template <typename T>
  concept is_streamable_type = is_stringlike_type<T> || streamable_type<T>;

  template <typename T>
  concept meets_core_reflection_requirements = std::default_initializable<T> && refl::is_reflectable<T>();

  template <typename T>
  concept reflected_type = meets_core_reflection_requirements<T> && has_type_data_handler<T>;

  template <typename T>
  constexpr static bool is_buffer_type = std::is_same_v<T, ostd::vector<uint8_t>>;

  template <typename T>
    requires reflected_type<T>
  auto get_type_name() {
    auto data = refl::reflect<T>();
    return std::string{ data.name };
  }

  template <typename T>
    requires(!reflected_type<T>)
  std::string get_type_name() {
    return std::string{ typeid(T).name() };
  }

  auto reflected_field_name(auto field_details) {
    std::string mname{ field_details.name };
    std::string dname{ refl::descriptor::get_attribute<attr::serializable>(field_details).display_name };
    return dname.empty() ? mname : dname;
  }

  struct string_writer {
    struct field_writer {
      template <typename U>
      constexpr void operator()(std::ostream& os, const U& value, uint32_t indent_level, const std::string& name, bool new_line = true) const {
        /// exclude linear algebra types from this because they look gross
        if constexpr (reflected_type<U> && !is_linear_algebra_type<U>) {
          os << string_writer{}.write_fields_to_string<U>(name, value, indent_level + 1);
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
              os << string_writer{}.write_fields_to_string<value_t>(name + "[" + std::to_string(count) + "]", val, indent_level + 1);
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
    };

    struct field_reader {
    };

    template <typename T>
      requires reflected_type<T>
    std::string write_fields_to_string(const std::string& name, const T& value, int32_t indent_level = 1) const;
  };

  class type_database : public subsystem<type_database> {
   public:
    type_database() = default;

    template <typename T>
      requires reflected_type<T>
    reflection_data* get_reflection_data();
    template <typename T>
      requires reflected_type<T>
    reflection_data* get_reflection_data(const T& value);

    bool has_type(const std::string_view type_name) const;
    const reflection_data* get_reflection_data(const std::string_view type_name);

    const ostd::map<uint64_t, reflection_data>& get_type_data() const {
      return data_map;
    }

   private:
    std::string get_namespace_string(const std::string_view full_name) const;
    std::string strip_namespace(const std::string_view full_name) const;

    ostd::map<uint64_t, reflection_data> data_map;
  };

  template <typename T>
    requires reflected_type<T>
  std::string string_writer::write_fields_to_string(const std::string& name, const T& value, int32_t indent_level) const {
    PROFILE_SECTION("string_writer::write_fields_to_string");
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
  reflection_data* type_database::get_reflection_data() {
    /// the value-taking overload checks the map before generating, so this is get-or-create
    return get_reflection_data(T{});
  }

  template <typename T>
    requires reflected_type<T>
  reflection_data* type_database::get_reflection_data(const T& value) {
    PROFILE_SECTION("type_database::get_reflection_data");
    static const auto refl_data = refl::reflect(value);
    static const std::string refl_type_name = std::string{ refl_data.name };
    static const uint64_t type_hash = typeid(T).hash_code();

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

    it->second.type_hash = type_hash;
    it->second.type_name = refl_type_name;
    for_each(refl::reflect(value).members, [&](auto member) {
      reflection_data::member m;

      m.type = reflection_data::member::FIELD;
      m.name = std::string{ member.name };

      if constexpr (refl::descriptor::is_function(member)) {
        m.type = reflection_data::member::FUNCTION;
      }

      using member_t = std::remove_cvref_t<decltype(member(value))>;
      m.value_type = get_value_type<member_t>();
      m.size = sizeof(member_t);

      if constexpr (!refl::descriptor::is_function(member)) {
        m.offset = reinterpret_cast<size_t>(&(static_cast<T*>(nullptr)->*member.pointer));
      }

      if constexpr (refl::descriptor::has_attribute<attr::serializable>(member)) {
        const auto& ser = refl::descriptor::get_attribute<attr::serializable>(member);
        m.flags |= reflection_data::SERIALIZABLE;
        m.flags |= ser.editable ?
          reflection_data::EDITABLE :
          reflection_data::READ_ONLY;

        if (!ser.display_name.empty()) {
          m.display_name = ser.display_name;
        }
      }

      if constexpr (refl::descriptor::has_attribute<attr::native_only>(member)) {
        m.flags |= reflection_data::NATIVE_ONLY;
      }

      if constexpr (refl::descriptor::has_attribute<attr::version_tag>(member)) {
        const auto& version = refl::descriptor::get_attribute<attr::version_tag>(member);
        m.since_version = version.version;
      }

      it->second.member_descriptors.push_back(m);
    });

    for_each(refl::reflect(value).bases, [&](auto base) { it->second.base_types.push_back(base.hash); });

    // CORE_LOG_TRACE("Added reflection data for type '{}'.", it->second.type_name);
    return &it->second;
  }

  template <typename T>
  std::string get_type_name_safe() {
    if constexpr (reflected_type<T>) {
      return get_type_name<T>();
    } else {
      return get_type_name<T>();
    }
  }

  // minimum size in bytes to serialize a value of type T,
  // type tag, 4 byte size, and the value itself, strings and buffers could be empty
  // so minimum is smaller than sizeof(value_type) + sizeof(uint32_t) + sizeof(T)
  template <typename T>
    requires(!is_stringlike_type<T> && !is_buffer_type<T>)
  static inline natural_t get_type_minimum_size() {
    return sizeof(value_type) + sizeof(uint32_t) + sizeof(T);
  }
  template <typename T>
    requires(is_stringlike_type<T> || is_buffer_type<T>)
  static inline natural_t get_type_minimum_size() {
    return sizeof(value_type) + sizeof(uint32_t);
  }

}  // namespace other

OTHER_DEPENDENT_SUBSYSTEM(
  other::type_database,
  subsystem_profile::kArena,
  subsystem_profile::kLogger);

#define VA_ARGS(...) , ##__VA_ARGS__

#define OTHER_REFLECTABLE(T)          \
  friend class other::type_database;  \
  friend struct other::string_writer; \
  friend struct other::type_data_handler<T>;

#define OTHER_TYPE_HANDLER(T)                                                                                                                                  \
  template <>                                                                                                                                                  \
  struct other::type_data_handler<T> {                                                                                                                         \
    static other::reflection_data& get_reflection_data(const T& value) { return *other::type_database::get()->get_reflection_data<T>(value); }                 \
    static std::string as_string(const T& value) { return other::string_writer{}.write_fields_to_string<T>(std::string{ refl::reflect(value).name }, value); } \
    static std::string as_string(const std::string& name, const T& value) { return other::string_writer{}.write_fields_to_string<T>(name, value); }            \
  };                                                                                                                                                           \
  static_assert(other::reflected_type<T>, "Type '" #T "' does not meet the requirements for reflection. Ensure it is default constructible and reflectable.");

#define OTHER_REFLECT(T, ...)             \
  REFL_AUTO(type(T) VA_ARGS(__VA_ARGS__)) \
  OTHER_TYPE_HANDLER(T)

#define OTHER_REFLECT_DERIVED(T, BT, ...) \
  REFL_AUTO(T, BT VA_ARGS(__VA_ARGS__))   \
  OTHER_TYPE_HANDLER(T)

#endif  // OTHER_CORE_REFLECTION_HPP
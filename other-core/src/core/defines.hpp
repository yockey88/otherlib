/**
 * @file core/defines.hpp
 */
#ifndef OTHER_CORE_CORE_DEFINES_HPP
#define OTHER_CORE_CORE_DEFINES_HPP

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <new>
#include <optional>
#include <span>
#include <string>
#include <type_traits>

#include <glm/fwd.hpp>

#include "core/build_config.hpp"
#include "data-structures/std_container.hpp"

#define bit(x) (1ll << x)

namespace other {

  constexpr static size_t kCacheLineSize = std::hardware_destructive_interference_size;

  template <typename T>
  concept is_pointer_type = std::is_pointer_v<std::remove_cvref_t<T>>;

  template <typename T>
  concept is_opaque_pointer = is_pointer_type<T> && std::is_same_v<std::remove_cvref_t<T>, void*>;
  template <typename T>
  concept is_byte_buffer_type =
    std::is_same_v<std::remove_cvref_t<T>, ostd::vector<uint8_t>> || std::is_same_v<std::remove_cvref_t<T>, std::span<const uint8_t>> ||
    std::is_same_v<std::remove_cvref_t<T>, std::span<uint8_t>> || std::is_same_v<std::remove_cvref_t<T>, std::vector<uint8_t>>;
  template <typename T>
  concept is_character_array_ptr = is_pointer_type<T> && std::is_same_v<std::remove_cvref_t<T>, char*>;
  template <typename T>
  concept is_bounded_character_array = std::is_array_v<std::remove_cvref_t<T>> || std::is_bounded_array_v<T>;
  template <typename T>
  concept is_character_array = is_character_array_ptr<T> || is_bounded_character_array<T>;
  template <typename T>
  concept is_string_type =
    std::is_same_v<std::remove_cvref_t<T>, std::string> ||
    std::is_same_v<std::remove_cvref_t<T>, std::string_view> ||
    is_character_array<T>;
  template <typename T>
  constexpr inline bool kIsStringType = is_string_type<T>;
  template <typename T>
  concept not_string_buffer_or_pointer = !is_pointer_type<T> && !is_string_type<T> && !is_byte_buffer_type<T> && !is_opaque_pointer<T>;
  template <typename T>
  concept string_buffer_table_or_pointer = !not_string_buffer_or_pointer<T>;

  template <typename T>
  concept is_acceptable_value_type = is_character_array<T> || !is_opaque_pointer<T> || is_byte_buffer_type<T>;

  enum exit_code : uint8_t {
    SUCCESS = 0,
    FAILURE = 1,

    /// others
    /// @note these are used for command exit codes as well in the other-command executor
    INVALID_COMMAND,
    INVALID_OPCODE,

    INVALID_ARGUMENT,
    MISSING_ARGUMENT,

    NUM_EXIT_CODES,
    INVALID_EXIT_CODE = NUM_EXIT_CODES
  };

#ifdef OTHER_USE_DOUBLE_FOR_REAL
  using real_t = double;
#else
  using real_t = float;
#endif  // OTHER_USE_DOUBLE_FOR_REAL

  using index_t = uint64_t;

  using natural_t = uint64_t;
  using integer_t = int64_t;

  template <typename T>
  using opt = std::optional<T>;

  using filepath = std::filesystem::path;

  template <typename T>
  struct result {
    inline bool is_err() const { return err.has_value(); }

    inline T& unwrap() { return value.value(); }
    inline const T& unwrap() const { return value.value(); }

    inline std::string& error() { return err.value(); }

   private:
    opt<T> value;
    opt<std::string> err;
  };

  enum value_type : uint8_t {
    /// primitive types
    OEBOOL,
    CHAR,
    INT8,
    INT16,
    INT32,
    INT64,
    UINT8,
    UINT16,
    UINT32,
    UINT64,
    FLOAT,
    DOUBLE,
    STRING,

    /// engine types
    VEC2,
    VEC3,
    VEC4,

    IVEC2,
    IVEC3,
    IVEC4,

    MAT2,
    MAT3,
    MAT4,

    QUATERNION,

    SAMPLER2D,
    SAMPLER2D_ARRAY,

    ASSET,
    ENTITY,

    /// user types
    USER_TYPE,
    OPAQUE_HANDLE,
    BYTE_BUFFER,

    /// error/misc
    EMPTY_TYPE,
  };

  template <typename T>
  static constexpr value_type get_value_type() {
    using no_cvref_t = std::remove_cvref_t<T>;
    if constexpr (std::is_same_v<no_cvref_t, bool>) {
      return value_type::OEBOOL;
    } else if constexpr (std::is_same_v<no_cvref_t, char>) {
      return value_type::CHAR;
    } else if constexpr (is_string_type<T>) {
      return value_type::STRING;
    } else if constexpr (std::is_same_v<no_cvref_t, int8_t>) {
      return value_type::INT8;
    } else if constexpr (std::is_same_v<no_cvref_t, int16_t>) {
      return value_type::INT16;
    } else if constexpr (std::is_same_v<no_cvref_t, int32_t>) {
      return value_type::INT32;
    } else if constexpr (std::is_same_v<no_cvref_t, integer_t>) {
      return value_type::INT64;
    } else if constexpr (std::is_same_v<no_cvref_t, uint8_t>) {
      return value_type::UINT8;
    } else if constexpr (std::is_same_v<no_cvref_t, uint16_t>) {
      return value_type::UINT16;
    } else if constexpr (std::is_same_v<no_cvref_t, uint32_t>) {
      return value_type::UINT32;
    } else if constexpr (std::is_same_v<no_cvref_t, natural_t>) {
      return value_type::UINT64;
    } else if constexpr (std::is_same_v<no_cvref_t, float>) {
      return value_type::FLOAT;
    } else if constexpr (std::is_same_v<no_cvref_t, double>) {
      return value_type::DOUBLE;
    } else if constexpr (std::is_same_v<no_cvref_t, glm::vec2>) {
      return value_type::VEC2;
    } else if constexpr (std::is_same_v<no_cvref_t, glm::vec3>) {
      return value_type::VEC3;
    } else if constexpr (std::is_same_v<no_cvref_t, glm::vec4>) {
      return value_type::VEC4;
    } else if constexpr (std::is_same_v<no_cvref_t, glm::ivec2>) {
      return value_type::IVEC2;
    } else if constexpr (std::is_same_v<no_cvref_t, glm::ivec3>) {
      return value_type::IVEC3;
    } else if constexpr (std::is_same_v<no_cvref_t, glm::ivec4>) {
      return value_type::IVEC4;
    } else if constexpr (std::is_same_v<no_cvref_t, glm::mat2>) {
      return value_type::MAT2;
    } else if constexpr (std::is_same_v<no_cvref_t, glm::mat3>) {
      return value_type::MAT3;
    } else if constexpr (std::is_same_v<no_cvref_t, glm::mat4>) {
      return value_type::MAT4;
    } else if constexpr (std::is_same_v<no_cvref_t, glm::quat>) {
      return value_type::QUATERNION;
    } else if constexpr (std::is_same_v<no_cvref_t, void*>) {
      return value_type::OPAQUE_HANDLE;
    } else if constexpr (is_byte_buffer_type<T>) {
      return value_type::BYTE_BUFFER;
    } else {
      return value_type::USER_TYPE;
    }
  }

  static inline value_type get_value_type_from_string(const std::string_view type_str) {
    std::string lc_str = std::string{ type_str };
    std::ranges::transform(lc_str, lc_str.begin(), [](unsigned char c) { return std::tolower(c); });

    if (lc_str == "bool") {
      return value_type::OEBOOL;
    } else if (lc_str == "char") {
      return value_type::CHAR;
    } else if (lc_str == "string") {
      return value_type::STRING;
    } else if (lc_str == "int8") {
      return value_type::INT8;
    } else if (lc_str == "int16") {
      return value_type::INT16;
    } else if (lc_str == "int32") {
      return value_type::INT32;
    } else if (lc_str == "int64") {
      return value_type::INT64;
    } else if (lc_str == "uint8") {
      return value_type::UINT8;
    } else if (lc_str == "uint16") {
      return value_type::UINT16;
    } else if (lc_str == "uint32") {
      return value_type::UINT32;
    } else if (lc_str == "uint64") {
      return value_type::UINT64;
    } else if (lc_str == "float") {
      return value_type::FLOAT;
    } else if (lc_str == "double") {
      return value_type::DOUBLE;
    } else if (lc_str == "vec2") {
      return value_type::VEC2;
    } else if (lc_str == "vec3") {
      return value_type::VEC3;
    } else if (lc_str == "vec4") {
      return value_type::VEC4;
    } else if (lc_str == "ivec2") {
      return value_type::IVEC2;
    } else if (lc_str == "ivec3") {
      return value_type::IVEC3;
    } else if (lc_str == "ivec4") {
      return value_type::IVEC4;
    } else if (lc_str == "mat2") {
      return value_type::MAT2;
    } else if (lc_str == "mat3") {
      return value_type::MAT3;
    } else if (lc_str == "mat4") {
      return value_type::MAT4;
    } else if (lc_str == "quaternion" || lc_str == "quat") {
      return value_type::QUATERNION;
    } else if (lc_str == "opaque-handle") {
      return value_type::OPAQUE_HANDLE;
    } else if (lc_str == "byte-buffer") {
      return value_type::BYTE_BUFFER;
    } else {
      return value_type::USER_TYPE;
    }
  }

  static inline std::string get_value_type_string_from_type(value_type type) {
    switch (type) {
      case value_type::OEBOOL: return "bool";
      case value_type::CHAR: return "char";
      case value_type::STRING: return "string";
      case value_type::INT8: return "int8";
      case value_type::INT16: return "int16";
      case value_type::INT32: return "int32";
      case value_type::INT64: return "int64";
      case value_type::UINT8: return "uint8";
      case value_type::UINT16: return "uint16";
      case value_type::UINT32: return "uint32";
      case value_type::UINT64: return "uint64";
      case value_type::FLOAT: return "float";
      case value_type::DOUBLE: return "double";
      case value_type::VEC2: return "vec2";
      case value_type::VEC3: return "vec3";
      case value_type::VEC4: return "vec4";
      case value_type::IVEC2: return "ivec2";
      case value_type::IVEC3: return "ivec3";
      case value_type::IVEC4: return "ivec4";
      case value_type::MAT2: return "mat2";
      case value_type::MAT3: return "mat3";
      case value_type::MAT4: return "mat4";
      case value_type::QUATERNION: return "quaternion";
      case value_type::OPAQUE_HANDLE: return "opaque-handle";
      case value_type::BYTE_BUFFER: return "byte-buffer";
      case value_type::USER_TYPE: return "user-type";
      default: return "unknown";
    }
  }

  filepath get_program_files_folder(const std::string_view app_name);
  filepath get_app_data_folder(const std::string_view app_name, bool create = false);
  filepath get_other_environment_install_folder();
  filepath get_system_default_working_directory();

  std::string get_tag_replacement(const std::string_view tag);
  std::string perform_tag_replacement(const std::string_view tag);
  std::string get_environment_build_config_string();
  std::string get_project_build_config_string();

  std::string get_current_exe_name();
  std::string get_current_exe_full_path();
  std::string get_current_exe_directory();
  std::string get_system_error_message();

}  // namespace other

#endif  // OTHER_CORE_CORE_DEFINES_HPP

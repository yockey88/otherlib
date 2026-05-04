/**
 * @file core/defines.hpp
 */
#ifndef OTHER_CORE_CORE_DEFINES_HPP
#define OTHER_CORE_CORE_DEFINES_HPP

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <format>
#include <optional>
#include <span>
#include <string>
#include <type_traits>

#define GLM_ENABLE_EXPERIMENTAL
// #define GLM_FORCE_QUAT_DATA_WXYZ
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <magic_enum/magic_enum.hpp>

#include <sol/sol.hpp>

#define bit(x) (1ll << x)

#ifdef OTHER_CLIENT
  #define OTHER_DYNAMIC_DRIVER
#elif defined(OTHER_APPLICATION) && !defined(OTHER_TEST_ENVIRONMENT)
  #define OTHER_STATIC_DRIVER
#elif defined(OTHER_PLUGIN_LIBRARY)
  #define OTHER_PLUGIN_DYNAMIC_LIBRARY
#else
  #define OTHER_STATIC_LIBRARY
#endif

#ifdef OTHER_ENVIRONMENT_WINDOWS
  #if defined(OTHER_CLIENT) || defined(OTHER_PLUGIN_LIBRARY)
    #define OTHER_API __declspec(dllexport)
    #define OTHER_CLASS __declspec(dllexport)
    #define OTHER_ALIGN(x) __declspec(align(x))
  #else
    #define OTHER_API
    #define OTHER_CLASS
    #define OTHER_ALIGN(x)
  #endif  // OTHER_CLIENT
#endif    // OTHER_ENVIRONMENT_WINDOWS

#ifdef OTHER_ENVIRONMENT_LINUX
  #ifdef OTHER_CLIENT
    #define OTHER_API __attribute__((visibility("default")))
    #define OTHER_CLASS __attribute__((visibility("default")))
    #define OTHER_ALIGN(x) __attribute__((aligned(x)))
  #else
    #define OTHER_API
    #define OTHER_CLASS
    #define OTHER_ALIGN(x)
  #endif  // OTHER_CLIENT
#endif    // OTHER_ENVIRONMENT_LINUX

#ifdef OTHER_ENVIRONMENT_DEBUG
  #define OTHER_DEBUG_BUILD
#endif  // !OTHER_DEBUG

#ifdef OTHER_ENVIRONMENT_RELEASE
  #define OTHER_RELEASE_BUILD
#endif  // !OTHER_RELEASE

#ifdef OTHER_ENVIRONMENT_PROFILE
  #define OTHER_PROFILE_BUILD
#endif  // !OTHER_PROFILE

#ifdef OTHER_ENVIRONMENT_PROFILED
  #define OTHER_PROFILED_BUILD
#endif  // !OTHER_PROFILED

#ifdef OTHER_ABORT_USE_STD_TERMINATE
  #include <cstdlib>
  #define OTHER_ABORT() std::terminate()
#else
  #define OTHER_ABORT() std::abort()
#endif  // !OTHER_ABORT_USE_STD_TERMINATE

#ifndef OTHER_API
  #error "OTHER_API is not defined. Please define it for your platform."
#endif  // !OTHER_API
#ifndef OTHER_CLASS
  #error "OTHER_CLASS is not defined. Please define it for your platform."
#endif  // !OTHER_CLASS
#ifndef OTHER_ALIGN
  #error "OTHER_ALIGN is not defined. Please define it for your platform."
#endif  // !OTHER_ALIGN

namespace other {

  template <typename T>
  concept is_pointer_type = std::is_pointer_v<std::remove_cvref_t<T>>;

  template <typename T>
  concept is_opaque_pointer = is_pointer_type<T> && std::is_same_v<std::remove_cvref_t<T>, void*>;
  template <typename T>
  concept is_byte_buffer_type = std::is_same_v<std::remove_cvref_t<T>, std::vector<uint8_t>> || std::is_same_v<std::remove_cvref_t<T>, std::span<const uint8_t>>;
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

  static inline size_t get_value_type_size(value_type type) {
    switch (type) {
      case value_type::OEBOOL: return sizeof(bool);
      case value_type::CHAR: return sizeof(char);
      case value_type::STRING: return 0;  /// string size is dynamic
      case value_type::INT8: return sizeof(int8_t);
      case value_type::INT16: return sizeof(int16_t);
      case value_type::INT32: return sizeof(int32_t);
      case value_type::INT64: return sizeof(int64_t);
      case value_type::UINT8: return sizeof(uint8_t);
      case value_type::UINT16: return sizeof(uint16_t);
      case value_type::UINT32: return sizeof(uint32_t);
      case value_type::UINT64: return sizeof(uint64_t);
      case value_type::FLOAT: return sizeof(float);
      case value_type::DOUBLE: return sizeof(double);
      case value_type::VEC2: return sizeof(glm::vec2);
      case value_type::VEC3: return sizeof(glm::vec3);
      case value_type::VEC4: return sizeof(glm::vec4);
      case value_type::IVEC2: return sizeof(glm::ivec2);
      case value_type::IVEC3: return sizeof(glm::ivec3);
      case value_type::IVEC4: return sizeof(glm::ivec4);
      case value_type::MAT2: return sizeof(glm::mat2);
      case value_type::MAT3: return sizeof(glm::mat3);
      case value_type::MAT4: return sizeof(glm::mat4);
      case value_type::QUATERNION: return sizeof(glm::quat);
      case value_type::OPAQUE_HANDLE: return sizeof(void*);
      case value_type::BYTE_BUFFER: return 0;
      default: return sizeof(void*);
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

  std::string get_current_exe_name();
  std::string get_current_exe_full_path();
  std::string get_system_error_message();
  void launch_process(const filepath& working_dir, const filepath& exe_name, const std::vector<std::string>& args);

}  // namespace other

namespace std {

  template <typename T>
    requires std::is_enum_v<T>
  struct formatter<T> : public formatter<std::string_view> {
    template <typename FormatContext>
    auto format(const T& value, FormatContext& ctx) const {
      auto enum_name = magic_enum::enum_name(value);
      if (enum_name.empty()) {
        return formatter<std::string_view>::format("Invalid enum value", ctx);
      }
      return formatter<std::string_view>::format(enum_name, ctx);
    }
  };

}  // namespace std

#endif  // OTHER_CORE_CORE_DEFINES_HPP

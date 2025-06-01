/**
 * @file core/defines.hpp
 */
#ifndef OTHER_CORE_CORE_DEFINES_HPP
#define OTHER_CORE_CORE_DEFINES_HPP

#include <cstdint>
#include <filesystem>
#include <format>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>

#include <glm/glm.hpp>
#include <magic_enum/magic_enum.hpp>

#define bit(x) (1ll << x)

#ifdef OTHER_MODULE
  #define OTHER_CLIENT
#else
  #define OTHER_CORE
#endif

#ifdef OTHER_ENVIRONMENT_WINDOWS
  #ifdef OTHER_CLIENT
    #define OTHER_API extern "C" __declspec(dllexport)
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

#ifdef OTHER_ENVIRONMENT_DEBUG_AS
  #define OTHER_DEBUG_AS
#endif  // !OTHER_DEBUG_AS

#ifdef OTHER_ENVIRONMENT_RELEASE
  #define OTHER_RELEASE_BUILD
#endif  // !OTHER_RELEASE

#ifdef OTHER_ENVIRONMENT_PROFILE
  #define OTHER_PROFILE_BUILD
#endif  // !OTHER_PROFILE

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

#ifdef OTHER_USE_DOUBLE_FOR_REAL
  using real_t = double;
#else
  using real_t = float;
#endif  // OTHER_USE_DOUBLE_FOR_REAL

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

  enum value_type {
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

    MAT2,
    MAT3,
    MAT4,

    SAMPLER2D,
    SAMPLER2D_ARRAY,

    ASSET,
    ENTITY,

    /// user types
    USER_TYPE,
    OPAQUE_HANDLE,

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
    } else if constexpr (std::is_same_v<no_cvref_t, std::string> || std::is_same_v<no_cvref_t, std::string_view>) {
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
    } else if constexpr (std::is_same_v<no_cvref_t, glm::mat2>) {
      return value_type::MAT2;
    } else if constexpr (std::is_same_v<no_cvref_t, glm::mat3>) {
      return value_type::MAT3;
    } else if constexpr (std::is_same_v<no_cvref_t, glm::mat4>) {
      return value_type::MAT4;
    } else if constexpr (std::is_same_v<no_cvref_t, void*>) {
      return value_type::OPAQUE_HANDLE;
    } else {
      return value_type::USER_TYPE;
    }
  }

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

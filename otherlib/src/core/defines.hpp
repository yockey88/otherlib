/**
 * \file defines.hpp
 */
#ifndef OTHER_CORE_DEFINES_HPP
#define OTHER_CORE_DEFINES_HPP

#include <cstdint>
#include <filesystem>
#include <format>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

#include <magic_enum/magic_enum.hpp>

#define OTHERENV_VERSION_MAJOR 0
#define OTHERENV_VERSION_MINOR 1
#define OTHERENV_VERSION_PATCH 0

#define OTHERENV_VERSION_STRING \
  std::format("{}.{}.{}", OTHERENV_VERSION_MAJOR, OTHERENV_VERSION_MINOR, OTHERENV_VERSION_PATCH)

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

#endif  // !OTHER_CORE_DEFINES_HPP

/**
 * \file defines.hpp
 */
#ifndef OTHER_CORE_DEFINES_HPP
#define OTHER_CORE_DEFINES_HPP

#include <magic_enum/magic_enum.hpp>

#include "core/fnv.hpp"

// #include <concepts>
#include <cstdint>
#include <filesystem>
#include <format>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

// #include <glm/glm.hpp>

#define OTHERENV_VERSION_MAJOR 0
#define OTHERENV_VERSION_MINOR 1
#define OTHERENV_VERSION_PATCH 0

#define OTHERENV_VERSION_STRING "0.1.0"

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
  #else
    #define OTHER_API
    #define OTHER_CLASS
  #endif  // OTHER_CLIENT
#endif    // OTHER_ENVIRONMENT_WINDOWS

#ifdef OTHER_ENVIRONMENT_LINUX
  #ifdef OTHER_CLIENT
    #define OTHER_API __attribute__((visibility("default")))
    #define OTHER_CLASS __attribute__((visibility("default")))
  #else
    #define OTHER_API
    #define OTHER_CLASS
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

namespace other {

#ifdef OTHER_USE_DOUBLE_FOR_REAL
  using real_t = double;
#else
  using real_t = float;
#endif  // OTHER_USE_DOUBLE_FOR_REAL

  using natural_t = uint64_t;
  using integer_t = int64_t;

  enum ExitCode : uint8_t {
    /// for os (program exit)
    SUCCESS = 0x00,
    FAILURE = 0x01,

    /// for internal use (reboot, reload, etc...)
    ///   internal good codes
    // RELOAD_PROJECT,
    // LOAD_NEW_PROJECT,
    // NO_EXIT,

    ///   internal bad codes
    // UNKNOWN_EXCEPTION,
    // NO_CONFIG_FILE,
    // CONFIG_PARSE_FAILURE,

    /// for user (config, etc...)
    // CORRUPT_CONFIGURATION,

    NUM_EXIT_CODES,
    INVALID = NUM_EXIT_CODES,
  };

  template <typename T>
  struct scope_deleter {
    void operator()(T* ptr) const {
      // PROFILE_DEALLOCATION(ptr);
      delete ptr;
    }
  };

  template <typename T>
  using scope_dtor = scope_deleter<T>;

  template <typename T>
  using scope = std::unique_ptr<T>;

  template <typename T>
  using opt = std::optional<T>;

  using filepath = std::filesystem::path;

  template <typename T, typename... Args>
  /// replace this allocation with arena allocator and maybe make a pool version of scope that takes a
  ///    memory pool to allocate into
  scope<T> make_scope(Args&&... args) {
    // PROFILE_ALLOCATION(memory, max_objects * sizeof(T));
    return std::make_unique<T>(std::forward<Args>(args)...);
  }

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

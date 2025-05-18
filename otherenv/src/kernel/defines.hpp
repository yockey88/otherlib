/**
 * \file kernel/defines.hpp
 */
#ifndef OTHERENV_CORE_DEFINES_HPP
#define OTHERENV_CORE_DEFINES_HPP

// #include <concepts>
#include <cstdint>
#include <filesystem>
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

#ifdef _WIN32
  #define OTHER_WINDOWS
  #ifdef OTHER_CLIENT
    #define OTHER_API extern "C" __declspec(dllexport)
    #define OTHER_CLASS __declspec(dllexport)
  #else
    #define OTHER_API
    #define OTHER_CLASS
  #endif  // !OTHER_CORE
#else
  #define OTHER_UNIX
  #ifdef OTHER_CLIENT
    #define OTHER_API __attribute__((visibility("default")))
    #define OTHER_CLASS __attribute__((visibility("default")))
  #else
    #define OTHER_API
    #define OTHER_CLASS
  #endif  // !OTHER_CORE
#endif    // !_WIN32

#ifdef OTHER_DEBUG
  #define OTHER_DEBUG_BUILD
#else
  #define OTHER_RELEASE_BUILD
#endif  // !OTHER_DEBUG

namespace other {

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

  enum ValueType {
    EMPTY_TYPE,  // Void , null , nil ,etc...

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
  };

  template <typename T>
  static constexpr ValueType GetValueType() {
    if constexpr (std::is_same_v<T, bool>) {
      return ValueType::OEBOOL;
    } else if constexpr (std::is_same_v<T, char>) {
      return ValueType::CHAR;
    } else if constexpr (std::is_same_v<T, std::string> || std::is_same_v<T, std::string_view>) {
      return ValueType::STRING;
    } else if constexpr (std::is_same_v<T, int8_t>) {
      return ValueType::INT8;
    } else if constexpr (std::is_same_v<T, int16_t>) {
      return ValueType::INT16;
    } else if constexpr (std::is_same_v<T, int32_t>) {
      return ValueType::INT32;
    } else if constexpr (std::is_same_v<T, int64_t>) {
      return ValueType::INT64;
    } else if constexpr (std::is_same_v<T, uint8_t>) {
      return ValueType::UINT8;
    } else if constexpr (std::is_same_v<T, uint16_t>) {
      return ValueType::UINT16;
    } else if constexpr (std::is_same_v<T, uint32_t>) {
      return ValueType::UINT32;
    } else if constexpr (std::is_same_v<T, uint64_t>) {
      return ValueType::UINT64;
    } else if constexpr (std::is_same_v<T, float>) {
      return ValueType::FLOAT;
    } else if constexpr (std::is_same_v<T, double>) {
      return ValueType::DOUBLE;
      // } else if constexpr (std::is_same_v<T, glm::vec2>) {
      //   return ValueType::VEC2;
      // } else if constexpr (std::is_same_v<T, glm::vec3>) {
      //   return ValueType::VEC3;
      // } else if constexpr (std::is_same_v<T, glm::vec4>) {
      //   return ValueType::VEC4;
      // } else if constexpr (std::is_same_v<T, glm::mat2>) {
      //   return ValueType::MAT2;
      // } else if constexpr (std::is_same_v<T, glm::mat3>) {
      //   return ValueType::MAT3;
      // } else if constexpr (std::is_same_v<T, glm::mat4>) {
      //   return ValueType::MAT4;
    } else if constexpr (std::is_same_v<T, void*>) {
      return ValueType::OPAQUE_HANDLE;
    } else {
      return ValueType::USER_TYPE;
    }
  }

  template <typename T>
  using Scope = std::unique_ptr<T>;

  template <typename T>
  using StdRef = std::shared_ptr<T>;

  template <typename T>
  using Opt = std::optional<T>;

  using Path = std::filesystem::path;

  template <typename T, typename... Args>
  Scope<T> NewScope(Args&&... args) {
    return std::make_unique<T>(std::forward<Args>(args)...);
  }

  template <typename T, typename... Args>
  StdRef<T> NewStdRef(Args&&... args) {
    return std::make_shared<T>(std::forward<Args>(args)...);
  }

  template <typename T>
  struct Result {
    Opt<T> value;
    Opt<std::string> error;

    inline bool IsOk() const { return value.has_value(); }
    inline bool IsErr() const { return error.has_value(); }

    inline T& Unwrap() { return value.value(); }
    inline const T& Unwrap() const { return value.value(); }

    inline std::string& Error() { return error.value(); }
  };

  static constexpr uint64_t kFnvOffsetBasis = 0xBCF29CE484222325;
  static constexpr uint64_t kFnvPrime = 0x100000001B3;

  constexpr uint64_t FNV(std::string_view str) {
    uint64_t hash = kFnvOffsetBasis;
    for (auto& c : str) {
      hash ^= c;
      hash *= kFnvPrime;
    }
    hash ^= str.length();
    hash *= kFnvPrime;

    return hash;
  }

}  // namespace other

#endif  // !OTHERENV_CORE_DEFINES_HPP

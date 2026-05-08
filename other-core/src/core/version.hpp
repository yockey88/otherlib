/**
 * @file core/version.hpp
 */
#ifndef OTHER_CORE_CORE_VERSION_HPP
#define OTHER_CORE_CORE_VERSION_HPP

#include <array>

#define OTHER_ENVIRONMENT_VERSION_MAJOR 0
#define OTHER_ENVIRONMENT_VERSION_MINOR 0
#define OTHER_ENVIRONMENT_VERSION_PATCH 1

#define OTHER_ENVIRONMENT_VERSION_STRING \
  std::format("{}.{}.{}", OTHER_ENVIRONMENT_VERSION_MAJOR, OTHER_ENVIRONMENT_VERSION_MINOR, OTHER_ENVIRONMENT_VERSION_PATCH)

namespace other {

  static inline constexpr std::array<uint8_t, 4> get_version_byte_header() {
    return { 0x01, 0x00, 0x00, 0x00 };  // Version header
  }

}  // namespace other

#endif  // OTHER_CORE_CORE_VERSION_HPP
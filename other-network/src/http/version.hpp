/**
 * \file http/version.hpp
 **/
#ifndef OTHER_NETWORK_HTTP_VERSION_HPP
#define OTHER_NETWORK_HTTP_VERSION_HPP

#include <array>
#include <cstdint>

namespace other {
  namespace http {

    struct version {
      int32_t major = 0;
      int32_t minor = 0;
      constexpr auto operator<=>(const version&) const = default;
    };

    constexpr static size_t kNumHttpVersions = 2;

    constexpr static version kHttpVersion1_0{ 1, 0 };
    constexpr static version kHttpVersion1_1{ 1, 1 };
    constexpr static std::array<version, kNumHttpVersions> supported_http_versions{
      kHttpVersion1_0,
      kHttpVersion1_1,
    };

  }  // namespace http
}  // namespace other

#endif  // OTHER_NETWORK_HTTP_VERSION_HPP
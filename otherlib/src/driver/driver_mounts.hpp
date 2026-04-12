/**
 * \file driver/driver_mounts.hpp
 **/
#ifndef OTHERLIB_DRIVER_DRIVER_MOUNTS_HPP
#define OTHERLIB_DRIVER_DRIVER_MOUNTS_HPP

#include <string_view>

namespace other {
  namespace driver_mounts {

    constexpr inline std::string_view kAssetMount = "assets";
    constexpr inline std::string_view kSceneMount = "scenes";
    constexpr inline std::string_view kScriptMount = "scripts";

  }  // namespace driver_mounts
}  // namespace other

#endif  // OTHERLIB_DRIVER_DRIVER_MOUNTS_HPP
/**
 * \file asset/asset.cpp
 **/
#include "asset/asset.hpp"

namespace other {

  asset::type asset::get_type_from_extension(const std::string_view extension) {
    for (const auto& asset_ext : kAssetExtensions) {
      if (asset_ext.extension == extension) {
        return asset_ext.asset_type;
      }
    }
    return asset::type::EMPTY;
  }

  std::vector<std::string> asset::get_supported_extensions(asset::type asset_type) {
    std::vector<std::string> extensions;
    for (const auto& asset_ext : kAssetExtensions) {
      if (asset_ext.asset_type == asset_type) {
        extensions.emplace_back(std::string{ asset_ext.extension });
      }
    }
    return extensions;
  }

}  // namespace other
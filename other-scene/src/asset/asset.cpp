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

  std::string asset::get_filesystem_directory() const {
    return get_filesystem_directory(asset_type);
  }

  std::string asset::get_filesystem_directory(asset::type asset_type) {
    switch (asset_type) {
      case asset::TEXTURE: return "textures";
      case asset::MODEL_SOURCE:
      case asset::MODEL:
        return "models";
      case asset::ANIMATION:
        return "animations";

      case asset::SCRIPT_SOURCE:
      case asset::SCRIPT:
        return "scripts";

      case asset::AUDIO:
        return "audio";

      case asset::SCENE:
        return "scenes";

      case asset::INPUT_MAP:
      case asset::RENDERING_PIPELINE:
      default:
        return "misc";
    }
  }

}  // namespace other
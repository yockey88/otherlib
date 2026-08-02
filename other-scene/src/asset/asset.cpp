/**
 * \file asset/asset.cpp
 **/
#include "asset/asset.hpp"

#include <toml++/toml.hpp>

#include "core/fnv.hpp"
#include "core/logger.hpp"
#include "file/path_helpers.hpp"

namespace other {

  domain_hit classify(const manifest_domain& d, const filepath& abs) {
    const opt<std::string> rel = try_relative(abs, d.root_abs);
    if (!rel.has_value()) {
      return domain_hit::OUTSIDE;
    }

    if (d.set.matches(*rel)) {
      return domain_hit::INSIDE;
    }

    if (d.set.excluded(*rel)) {
      return domain_hit::EXCLUDED;
    }

    return domain_hit::OUTSIDE;
  }

  asset::type asset::get_type_from_extension(const std::string_view extension) {
    for (const auto& asset_ext : kAssetExtensions) {
      if (asset_ext.extension == extension) {
        return asset_ext.asset_type;
      }
    }
    return asset::type::EMPTY;
  }

  asset::type asset::get_type_from_declaration(const filepath& file_path) {
    toml::table t;
    try {
      t = toml::parse_file(file_path.string());
    } catch (const toml::parse_error& e) {
      CORE_LOG_ERROR("TOML parse error in file: {}: {}", file_path.string(), e.what());
      return asset::type::EMPTY;
    } catch (const std::exception& e) {
      CORE_LOG_ERROR("Exception occurred while parsing TOML file: {}: {}", file_path.string(), e.what());
      return asset::type::EMPTY;
    } catch (...) {
      CORE_LOG_ERROR("Unknown exception occurred while parsing TOML file: {}", file_path.string());
      return asset::type::EMPTY;
    }

    auto type = t.at_path("asset-type");
    if (!type) {
      CORE_LOG_ERROR("Asset type not specified in TOML file: {}", file_path.string());
      return asset::type::EMPTY;
    }

    if (!type.is_string()) {
      CORE_LOG_ERROR("Asset type in TOML file is not a string: {}", file_path.string());
      CORE_LOG_ERROR("type type: {}", type.type());
      return asset::type::EMPTY;
    }

    std::string type_str = type.as_string()->get();
    switch (FNV(type_str)) {
      case FNV("rendering-pipeline"): return asset::type::RENDERING_PIPELINE;
      case FNV("scene"): return asset::type::SCENE;
      default:
        CORE_LOG_ERROR("asset type {} not supported through TOML file: {}", type_str, file_path.string());
        return asset::type::EMPTY;
    }

    OTHER_ASSERT(false, "Unknown asset type specified in TOML file: {}", file_path.string());
  }

  std::string asset::get_name_from_declaration(const filepath& file_path) {
    toml::table t;
    try {
      t = toml::parse_file(file_path.string());
    } catch (const toml::parse_error& e) {
      CORE_LOG_ERROR("TOML parse error in file: {}: {}", file_path.string(), e.what());
      return "";
    } catch (const std::exception& e) {
      CORE_LOG_ERROR("Exception occurred while parsing TOML file: {}: {}", file_path.string(), e.what());
      return "";
    } catch (...) {
      CORE_LOG_ERROR("Unknown exception occurred while parsing TOML file: {}", file_path.string());
      return "";
    }

    auto n = t.at_path("name");
    if (!n) {
      CORE_LOG_ERROR("Asset name not specified in TOML file: {}", file_path.string());
      return "";
    }

    if (!n.is_string()) {
      CORE_LOG_ERROR("Asset name in TOML file is not a string: {}", file_path.string());
      CORE_LOG_ERROR("name type: {}", n.type());
      return "";
    }

    return n.as_string()->get();
  }

  ostd::vector<std::string> asset::get_supported_extensions(asset::type asset_type) {
    ostd::vector<std::string> extensions;
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

      case asset::SCRIPT_PROJECT:
      case asset::SCRIPT_SOURCE:
      case asset::SCRIPT_FILE:
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
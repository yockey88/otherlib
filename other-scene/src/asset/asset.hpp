/**
 * \file asset/asset.hpp
 **/
#ifndef OTHER_SCENE_ASSET_ASSET_HPP
#define OTHER_SCENE_ASSET_ASSET_HPP

#include <array>

#include "core/defines.hpp"

#include "model/model.hpp"

namespace other {

  struct asset {
    enum type {
      TEXTURE = 0,

      MODEL_SOURCE,
      MODEL,

      SCRIPT_SOURCE,
      SCRIPT,

      AUDIO,

      EMPTY,
      NUM_ASSET_TYPES = EMPTY,
    };

    type asset_type = type::EMPTY;

    natural_t id = 0;
    natural_t path_hash = 0;
    filepath path = "";

    static asset::type get_type_from_extension(const std::string_view extension);
    static std::vector<std::string> get_supported_extensions(asset::type asset_type);
  };

  struct asset_extension {
    asset::type asset_type;
    std::string_view extension;
  };
  constexpr inline std::array<std::string_view, 10> kFileExtensions = {
    ".jpg",  // TEXTURE
    ".png",  // TEXTURE

    ".omesh",  // MODEL_SOURCE
    ".fbx",    // MODEL_SOURCE
    ".obj",    // MODEL_SOURCE

    ".cs",   // SCRIPT_SOURCE
    ".lua",  // SCRIPT_SOURCE
    ".py",   // SCRIPT_SOURCE

    ".mp3",  // AUDIO
    ".wav",  // AUDIO
  };

  constexpr inline std::array<asset_extension, kFileExtensions.size()> kAssetExtensions{
    {
      { asset::TEXTURE, ".jpg" },
      { asset::TEXTURE, ".png" },

      { asset::MODEL_SOURCE, ".omesh" },
      { asset::MODEL_SOURCE, ".fbx" },
      { asset::MODEL_SOURCE, ".obj" },

      { asset::SCRIPT_SOURCE, ".cs" },
      { asset::SCRIPT_SOURCE, ".lua" },
      { asset::SCRIPT_SOURCE, ".py" },

      { asset::AUDIO, ".mp3" },
      { asset::AUDIO, ".wav" },
    }
  };

  struct model_source_asset {
    ref<model_source> source = nullptr;
    natural_t asset_id;
  };

  struct model_asset {
    // ref<model> model_ptr = nullptr;
    // natural_t asset_id;
  };

}  // namespace other

#endif  // OTHER_SCENE_ASSET_ASSET_HPP
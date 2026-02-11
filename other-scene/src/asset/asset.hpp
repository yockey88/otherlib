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
      ANIMATION,

      SCRIPT_SOURCE,
      SCRIPT,

      AUDIO,

      SCENE,
      SCENE_OBJECT,

      INPUT_MAP,

      EMPTY,
      NUM_ASSET_TYPES = EMPTY,
    };

    type asset_type = type::EMPTY;

    natural_t id = 0;

    /// hash uses absolute path string to avoid issues with relative paths and different working directories
    natural_t path_hash = 0;

    filepath path = "";
    filepath absolute_path = "";

    static asset::type get_type_from_extension(const std::string_view extension);
    static std::vector<std::string> get_supported_extensions(asset::type asset_type);
  };

  namespace attr {

    struct asset_identifier_field : refl::attr::usage::field {
      asset::type asset_type = asset::type::EMPTY;
      constexpr asset_identifier_field(asset::type type)
          : asset_type(type) {}
    };

  }  // namespace attr

  struct asset_extension {
    asset::type asset_type;
    std::string_view extension;
  };

  constexpr inline std::array<std::string_view, 13> kFileExtensions = {
    ".jpg",  // TEXTURE
    ".png",  // TEXTURE

    ".omesh",  // MODEL_SOURCE
    ".fbx",    // MODEL_SOURCE
    ".obj",    // MODEL_SOURCE

    /// usually actually just loaded from fbx with model source
    ".anim",  // ANIMATION

    ".cs",   // SCRIPT_SOURCE
    ".lua",  // SCRIPT_SOURCE
    ".py",   // SCRIPT_SOURCE

    ".mp3",  // AUDIO
    ".wav",  // AUDIO

    // ".scene",         // SCENE
    // ".scene-object",  // SCENE_OBJECT

    ".oinputmap",  // INPUT_MAP
    ".oeim",
  };

  constexpr inline std::array<asset_extension, kFileExtensions.size()> kAssetExtensions{
    {
      { asset::TEXTURE, ".jpg" },
      { asset::TEXTURE, ".png" },

      { asset::MODEL_SOURCE, ".omesh" },
      { asset::MODEL_SOURCE, ".fbx" },
      { asset::MODEL_SOURCE, ".obj" },

      { asset::ANIMATION, ".anim" },

      { asset::SCRIPT_SOURCE, ".cs" },
      { asset::SCRIPT_SOURCE, ".lua" },
      { asset::SCRIPT_SOURCE, ".py" },

      { asset::AUDIO, ".mp3" },
      { asset::AUDIO, ".wav" },

      { asset::INPUT_MAP, ".oinputmap" },
      { asset::INPUT_MAP, ".oeim" },
    }
  };

}  // namespace other

#endif  // OTHER_SCENE_ASSET_ASSET_HPP
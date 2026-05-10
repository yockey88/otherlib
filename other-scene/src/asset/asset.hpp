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

      SCRIPT_PROJECT,
      SCRIPT_SOURCE,
      SCRIPT_FILE,
      SCRIPT,

      AUDIO,

      SCENE,

      INPUT_MAP,
      RENDERING_PIPELINE,

      EMPTY,
      NUM_ASSET_TYPES,
    };

    type asset_type = type::EMPTY;

    natural_t id = 0;
    natural_t parent_id = 0;

    /// hash uses absolute path string to avoid issues with relative paths and different working directories
    natural_t path_hash = 0;

    /// load path is the path passed to load_asset, may be relative or absolute,
    filepath load_path = "";
    /// virtual path is resolved via the filesystem mounts
    ///  we use string to help avoid confusion since virtual_path is probably not a real filesystem path (although it can be)
    std::string virtual_path = "";
    filepath absolute_path = "";

    static asset::type get_type_from_extension(const std::string_view extension);
    static std::vector<std::string> get_supported_extensions(asset::type asset_type);

    std::string get_filesystem_directory() const;
    static std::string get_filesystem_directory(asset::type asset_type);
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

  constexpr inline size_t kNumAssetExtensions = 17;
  constexpr inline std::array<std::string_view, kNumAssetExtensions> kFileExtensions = {
    ".jpg",  // TEXTURE
    ".png",  // TEXTURE

    ".omesh",  // MODEL_SOURCE
    ".fbx",    // MODEL_SOURCE
    ".obj",    // MODEL_SOURCE

    /// usually actually just loaded from fbx with model source
    ".anim",  // ANIMATION

    ".csproj",  // SCRIPT_PROJECT
    ".cs",      // SCRIPT_FILE
    ".dll",     // SCRIPT_SOURCE
    ".so",      // SCRIPT_SOURCE

    /// no real extension since scripts can be anything loaded out of a script source
    ".os",  // SCRIPT

    ".mp3",  // AUDIO
    ".wav",  // AUDIO

    /// fix this so that we load lua as SCRIPT_FILE
    //  involves fixing scene loading
    ".lua",  // SCENE
    // ".scene",         // SCENE
    // ".scene-object",  // SCENE_OBJECT

    ".oinputmap",  // INPUT_MAP
    ".oeim",       // INPUT_MAP

    ".orpl",  // RENDERING_PIPELINE
  };

  constexpr inline std::array<asset_extension, kNumAssetExtensions> kAssetExtensions{
    {
      { asset::TEXTURE, ".jpg" },
      { asset::TEXTURE, ".png" },

      { asset::MODEL_SOURCE, ".omesh" },
      { asset::MODEL_SOURCE, ".fbx" },
      { asset::MODEL_SOURCE, ".obj" },

      { asset::ANIMATION, ".anim" },

      { asset::SCRIPT_PROJECT, ".csproj" },
      { asset::SCRIPT_SOURCE, ".dll" },
      { asset::SCRIPT_SOURCE, ".so" },
      { asset::SCRIPT_FILE, ".cs" },

      { asset::AUDIO, ".mp3" },
      { asset::AUDIO, ".wav" },

      { asset::SCENE, ".lua" },
      { asset::INPUT_MAP, ".oinputmap" },
      { asset::INPUT_MAP, ".oeim" },

      { asset::RENDERING_PIPELINE, ".orpl" },
    }
  };

}  // namespace other

#endif  // OTHER_SCENE_ASSET_ASSET_HPP
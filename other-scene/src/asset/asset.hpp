/**
 * \file asset/asset.hpp
 **/
#ifndef OTHER_SCENE_ASSET_ASSET_HPP
#define OTHER_SCENE_ASSET_ASSET_HPP

#include <array>

#include <refl/refl.hpp>

#include <xxHash/xxh3.h>

#include "core/defines.hpp"
#include "core/enum_formatter.hpp"
#include "file/glob.hpp"

namespace other {

  inline static natural_t stable_id_for(const std::string_view virtual_path) {
    OTHER_ASSERT(!virtual_path.empty(), "empty virtual path");
    OTHER_ASSERT(virtual_path.find('\\') == std::string_view::npos, "virtual paths use forward slashes: '{}'", virtual_path);
    OTHER_ASSERT(virtual_path.find("..") == std::string_view::npos && virtual_path.find("./") == std::string_view::npos, "virtual paths must be normalized: '{}'", virtual_path);
    OTHER_ASSERT(virtual_path.front() != '/', "virtual paths are mount-relative: '{}'", virtual_path);
    return XXH3_64bits(virtual_path.data(), virtual_path.size());
  }

  struct manifest_domain {
    natural_t owner_stable_id = 0;
    filepath root_abs;
    glob_set set;
  };

  enum class domain_hit : uint8_t {
    OUTSIDE,   /// not this domain's concern
    EXCLUDED,  /// inside the root but matches the exclude set (bin/, obj/, ...)
    INSIDE,    /// attributable to the owner
  };

  domain_hit classify(const manifest_domain& d, const filepath& abs);

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

      ASSET_DECLARATION,

      EMPTY,
      NUM_ASSET_TYPES,
    };

    type asset_type = type::EMPTY;

    natural_t id = 0;
    natural_t stable_id = 0;

    /// hash uses absolute path string to avoid issues with relative paths and different working directories
    natural_t path_hash = 0;

    /// load path is the path passed to load_asset, may be relative or absolute,
    filepath load_path = "";
    /// virtual path is resolved via the filesystem mounts
    ///  we use string to help avoid confusion since virtual_path is probably not a real filesystem path (although it can be)
    filepath virtual_path = "";
    filepath absolute_path = "";

    static asset::type get_type_from_extension(const std::string_view extension);
    static asset::type get_type_from_declaration(const filepath& file_path);
    static std::string get_name_from_declaration(const filepath& file_path);
    static ostd::vector<std::string> get_supported_extensions(asset::type asset_type);

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

  struct asset_name {
    const std::string_view tag_name;
    const std::string_view display_name;
    constexpr asset_name(const std::string_view tag, const std::string_view display)
        : tag_name(tag), display_name(display) {}
  };

  struct asset_extension {
    asset::type asset_type;
    std::string_view extension;
  };

  constexpr inline size_t kNumAssetTypes = static_cast<size_t>(asset::type::NUM_ASSET_TYPES);
  constexpr inline std::array<asset_name, kNumAssetTypes> kAssetTypeNames = {
    asset_name{ "texture", "Texture" },

    asset_name{ "model-source", "Model Source" },
    asset_name{ "model", "Model" },
    asset_name{ "animation", "Animation" },

    asset_name{ "script-project", "Script Project" },
    asset_name{ "script-source", "Script Source" },
    asset_name{ "script-file", "Script File" },
    asset_name{ "script", "Script" },

    asset_name{ "audio", "Audio" },

    asset_name{ "scene", "Scene" },

    asset_name{ "input-map", "Input Map" },
    asset_name{ "rendering-pipeline", "Rendering Pipeline" },

    asset_name{ "asset-declaration", "Asset Declaration" },

    asset_name{ "empty", "Empty" },
  };

  constexpr inline size_t kNumAssetExtensions = 20;
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
    /// scene behavior-hook scripts and projectrc files; scenes themselves are .oscn/.oscnb
    ".lua",  // SCRIPT_FILE
    ".dll",  // SCRIPT_SOURCE
    ".so",   // SCRIPT_SOURCE

    /// no real extension since scripts can be anything loaded out of a script source
    ".os",  // SCRIPT

    ".mp3",  // AUDIO
    ".wav",  // AUDIO

    ".oscn",   // SCENE (toml scene document)
    ".oscnb",  // SCENE (compiled binary scene document)

    ".oinputmap",  // INPUT_MAP
    ".oeim",       // INPUT_MAP

    ".orpl",  // RENDERING_PIPELINE

    ".toml"
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
      { asset::SCRIPT_FILE, ".lua" },

      { asset::SCRIPT, ".os" },

      { asset::AUDIO, ".mp3" },
      { asset::AUDIO, ".wav" },

      { asset::SCENE, ".oscn" },
      { asset::SCENE, ".oscnb" },
      { asset::INPUT_MAP, ".oinputmap" },
      { asset::INPUT_MAP, ".oeim" },

      { asset::RENDERING_PIPELINE, ".orpl" },

      { asset::ASSET_DECLARATION, ".toml" },
    }
  };

  constexpr std::string_view get_asset_type_tag_name(asset::type type) {
    OTHER_ASSERT(type >= 0 && type < asset::type::NUM_ASSET_TYPES, "Invalid asset type: {}", type);
    return kAssetTypeNames[static_cast<size_t>(type)].tag_name;
  }

  static inline std::string get_asset_event_name(asset::type type, const std::string_view event) {
    OTHER_ASSERT(type >= 0 && type < asset::type::NUM_ASSET_TYPES, "Invalid asset type: {}", type);
    return std::format("{}.{}", get_asset_type_tag_name(type), event);
  }

}  // namespace other

#endif  // OTHER_SCENE_ASSET_ASSET_HPP
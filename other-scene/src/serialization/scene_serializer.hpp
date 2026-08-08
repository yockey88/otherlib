/**
 * \file serialization/scene_serializer.hpp
 *
 * scene <-> scene_document <-> (.oscn toml | .oscnb binary). the binary form doubles
 * as the in-memory snapshot wire format (scene::capture_snapshot / restore_snapshot),
 * which is what play/stop restore, editor undo/redo, and future state replication ride.
 **/
#ifndef OTHER_SCENE_SERIALIZATION_SCENE_SERIALIZER_HPP
#define OTHER_SCENE_SERIALIZATION_SCENE_SERIALIZER_HPP

#include "core/defines.hpp"

#include "serialization/component_codec.hpp"
#include "serialization/scene_document.hpp"

namespace other {

  class scene;

  namespace serialization {

    constexpr inline std::string_view kSceneTomlExtension = ".oscn";
    constexpr inline std::string_view kSceneBinaryExtension = ".oscnb";

    inline bool is_scene_file_extension(const std::string_view extension) {
      return extension == kSceneTomlExtension || extension == kSceneBinaryExtension;
    }

    /// -- live scene <-> document -------------------------------------------

    /// walks the scene tree depth-first (parents before children); the root object is
    /// not captured — scenes construct their own root
    scene_document capture_scene(scene& s, const codec_services& services);

    /// instantiates the document's objects into @p s through the normal creation APIs,
    /// so entt construct signals rebuild script objects and physics bodies; file ids
    /// are remapped to freshly allocated runtime ids (written to @p out_id_remap when
    /// given — join snapshots adopt net identities through it)
    void instantiate_scene(scene& s, const scene_document& doc, const codec_services& services,
                           ostd::map<natural_t, natural_t>* out_id_remap = nullptr);

    /// -- document <-> bytes (.oscnb + snapshots) ---------------------------

    ostd::vector<uint8_t> write_scene_binary(const scene_document& doc);
    scene_parse_result parse_scene_binary(std::span<const uint8_t> bytes);

    /// -- document <-> toml (.oscn) -----------------------------------------

    std::string write_scene_toml(const scene_document& doc);
    scene_parse_result parse_scene_toml(std::string_view text);

    /// -- file helpers (dispatch on extension) ------------------------------

    scene_parse_result load_scene_document(const filepath& path);
    bool save_scene_document(const scene_document& doc, const filepath& path);

    /// every asset reference the document's component payloads carry, as stored;
    /// the scene's hook script is separate (scene_document::script, relative to the
    /// scene file's directory). engine-free — this is what manifest parsing rides
    ostd::vector<component_asset_ref> collect_scene_asset_refs(const scene_document& doc);

  }  // namespace serialization
}  // namespace other

#endif  // OTHER_SCENE_SERIALIZATION_SCENE_SERIALIZER_HPP

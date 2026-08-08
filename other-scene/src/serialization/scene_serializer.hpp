/**
 * \file serialization/scene_serializer.hpp
 *
 * scene <-> scene_document <-> (.oscn toml | .oscnb binary); the binary form doubles as the in-memory
 * snapshot wire format (capture/restore_snapshot) used by play/stop restore, undo/redo, and replication
 **/
#ifndef OTHER_SCENE_SERIALIZATION_SCENE_SERIALIZER_HPP
#define OTHER_SCENE_SERIALIZATION_SCENE_SERIALIZER_HPP

#include "core/defines.hpp"

#include "serialization/component_codec.hpp"
#include "serialization/scene_document.hpp"

namespace other {

  class scene;
  struct scene_object;

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

    /// instantiates the document's objects into @p s via normal creation APIs (entt construct signals
    /// rebuild scripts/physics); file ids remap to fresh runtime ids, written to @p out_id_remap for join snapshots
    void instantiate_scene(scene& s, const scene_document& doc, const codec_services& services,
                           ostd::map<natural_t, natural_t>* out_id_remap = nullptr);

    /// -- object subtrees (spawn templates, duplicate-object, prefabs) ------

    /// one object and its descendants through the same codecs; the subtree root's
    /// record carries parent_file_id 0
    scene_document capture_object_subtree(scene& s, natural_t root_object_id, const codec_services& services);

    /// instantiates the document's records under @p parent (nullptr = scene root)
    void instantiate_subtree(scene& s, const scene_document& doc, scene_object* parent, const codec_services& services,
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

    /// every asset reference the document's component payloads carry, as stored (hook script is
    /// separate: scene_document::script); engine-free — this is what manifest parsing rides
    ostd::vector<component_asset_ref> collect_scene_asset_refs(const scene_document& doc);

  }  // namespace serialization
}  // namespace other

#endif  // OTHER_SCENE_SERIALIZATION_SCENE_SERIALIZER_HPP

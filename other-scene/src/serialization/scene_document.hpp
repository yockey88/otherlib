/**
 * \file serialization/scene_document.hpp
 *
 * plain-data intermediate every scene persistence path converts through (live scenes, .oscn, .oscnb,
 * snapshots, cli); component state rides field-tagged binary payloads (scene_field_codec.hpp) — one canonical encoding
 */
#ifndef OTHER_SCENE_SERIALIZATION_SCENE_DOCUMENT_HPP
#define OTHER_SCENE_SERIALIZATION_SCENE_DOCUMENT_HPP

#include <glm/glm.hpp>

#include "core/defines.hpp"

namespace other {
  namespace serialization {

    struct component_record {
      /// FNV of the component codec key ("transform", "render", ...)
      natural_t key_hash = 0;
      /// field stream, see scene_field_codec.hpp
      ostd::vector<uint8_t> payload = {};
    };

    struct object_record {
      /// unique within the document; instantiation remaps to freshly allocated runtime ids
      natural_t file_id = 0;
      /// file_id of the parent record, 0 = scene root
      natural_t parent_file_id = 0;
      std::string name = "";
      bool visible = true;
      ostd::vector<std::string> tags = {};
      ostd::vector<component_record> components = {};
    };

    struct scene_document {
      constexpr static uint32_t kCurrentSchemaVersion = 1;

      uint32_t schema_version = kCurrentSchemaVersion;
      std::string name = "";
      /// optional behavior-hooks lua script, relative to the scene file's directory
      std::string script = "";
      glm::vec4 clear_color = glm::vec4(0.2f, 0.22f, 0.233f, 1.0f);
      /// parent records always precede their children
      ostd::vector<object_record> objects = {};
    };

    /// parse results never assert on malformed input — scene files and snapshots are data,
    ///  not programmer contracts; hosts (loader, cli, editor) decide how to surface errors
    struct scene_parse_result {
      opt<scene_document> document = std::nullopt;
      std::string error = "";
      ostd::vector<std::string> warnings = {};

      inline bool success() const { return document.has_value(); }

      static inline scene_parse_result ok(scene_document doc, ostd::vector<std::string> warnings = {}) {
        return { .document = std::move(doc), .error = "", .warnings = std::move(warnings) };
      }
      static inline scene_parse_result fail(std::string message) {
        return { .document = std::nullopt, .error = std::move(message), .warnings = {} };
      }
    };

  }  // namespace serialization
}  // namespace other

#endif  // OTHER_SCENE_SERIALIZATION_SCENE_DOCUMENT_HPP

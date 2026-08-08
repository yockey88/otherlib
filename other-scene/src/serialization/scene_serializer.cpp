/**
 * \file serialization/scene_serializer.cpp
 **/
#include "serialization/scene_serializer.hpp"

#include <fstream>

#include "core/logger.hpp"
#include "core/profiler.hpp"
#include "serialization/scene_field_codec.hpp"

#include "scene/scene.hpp"

namespace other {
  namespace serialization {
    namespace {

      constexpr std::array<uint8_t, 4> kSceneBinaryMagic = { 'O', 'S', 'C', 'N' };
      constexpr uint16_t kSceneBinaryFormatVersion = 1;

      void capture_object_recursive(scene& s, natural_t object_id, natural_t parent_file_id, const codec_services& services, scene_document& doc) {
        scene_object* object = s.find_object(object_id);
        OTHER_ASSERT(object != nullptr, "scene tree yielded an id without an object ({})", object_id);

        object_record record = {
          .file_id = object->id,
          .parent_file_id = parent_file_id,
          .name = object->name,
          .visible = object->visible,
          .tags = s.get_object_tags(object->id),
          .components = {},
        };

        for (const component_codec& codec : component_codecs()) {
          if (!codec.has(s, object)) {
            continue;
          }
          ostd::vector<uint8_t> payload = codec.capture(s, object, services);
          if (codec.implicit && payload.empty()) {
            continue;
          }
          record.components.push_back(component_record{ .key_hash = codec.key_hash, .payload = std::move(payload) });
        }

        doc.objects.push_back(std::move(record));

        for (const natural_t child_id : s.get_children_ids(object_id)) {
          capture_object_recursive(s, child_id, object->id, services, doc);
        }
      }

    }  // namespace

    scene_document capture_scene(scene& s, const codec_services& services) {
      PROFILE_SECTION("capture_scene");
      scene_document doc = {};
      doc.schema_version = scene_document::kCurrentSchemaVersion;
      doc.name = s.name;
      doc.script = s.script_source;
      doc.clear_color = s.get_storage().clear_color;

      const natural_t root_id = s.root_object().id;
      for (const natural_t child_id : s.get_children_ids(root_id)) {
        capture_object_recursive(s, child_id, /*parent_file_id=*/0, services, doc);
      }
      return doc;
    }

    namespace {

      /// the shared instantiation loop: records with parent_file_id 0 land under
      /// @p root_parent (nullptr = scene root)
      void instantiate_records(scene& s, const scene_document& doc, scene_object* root_parent,
                               const codec_services& services, ostd::map<natural_t, natural_t>& id_remap) {
        for (const object_record& record : doc.objects) {
          scene_object* parent = root_parent;
          if (record.parent_file_id != 0) {
            const auto it = id_remap.find(record.parent_file_id);
            if (it == id_remap.end()) {
              CORE_LOG_ERROR("scene document object '{}' references unknown parent {:#x}; parenting to root", record.name, record.parent_file_id);
            } else {
              parent = s.find_object(it->second);
            }
          }

          scene_object& object = s.create_object(record.name, parent);
          id_remap[record.file_id] = object.id;
          object.visible = record.visible;
          for (const std::string& tag : record.tags) {
            s.add_object_tag(object.id, tag);
          }

          for (const component_record& component : record.components) {
            const component_codec* codec = find_component_codec(component.key_hash);
            if (codec == nullptr) {
              CORE_LOG_WARN("scene document object '{}' carries unknown component {:#018x}; skipped", record.name, component.key_hash);
              continue;
            }
            codec->apply(s, &object, component.payload, services);
          }
        }
      }

    }  // namespace

    void instantiate_scene(scene& s, const scene_document& doc, const codec_services& services,
                           ostd::map<natural_t, natural_t>* out_id_remap) {
      PROFILE_SECTION("instantiate_scene");
      s.get_storage().clear_color = doc.clear_color;

      ostd::map<natural_t, natural_t> id_remap = {};
      instantiate_records(s, doc, nullptr, services, id_remap);
      if (out_id_remap != nullptr) {
        *out_id_remap = std::move(id_remap);
      }
    }

    scene_document capture_object_subtree(scene& s, natural_t root_object_id, const codec_services& services) {
      PROFILE_SECTION("capture_object_subtree");
      scene_document doc = {};
      doc.schema_version = scene_document::kCurrentSchemaVersion;
      scene_object* root = s.find_object(root_object_id);
      if (root == nullptr) {
        CORE_LOG_ERROR("capture_object_subtree: no object {}", root_object_id);
        return doc;
      }
      doc.name = root->name;
      capture_object_recursive(s, root_object_id, /*parent_file_id=*/0, services, doc);
      return doc;
    }

    void instantiate_subtree(scene& s, const scene_document& doc, scene_object* parent, const codec_services& services,
                             ostd::map<natural_t, natural_t>* out_id_remap) {
      PROFILE_SECTION("instantiate_subtree");
      ostd::map<natural_t, natural_t> id_remap = {};
      instantiate_records(s, doc, parent, services, id_remap);
      if (out_id_remap != nullptr) {
        *out_id_remap = std::move(id_remap);
      }
    }

    /// -- binary ------------------------------------------------------------

    ostd::vector<uint8_t> write_scene_binary(const scene_document& doc) {
      PROFILE_SECTION("write_scene_binary");
      using namespace field_codec;

      ostd::vector<uint8_t> out = {};
      out.insert(out.end(), kSceneBinaryMagic.begin(), kSceneBinaryMagic.end());
      write_raw<uint16_t>(kSceneBinaryFormatVersion, out);
      write_raw<uint16_t>(0, out);  /// flags
      write_raw<uint32_t>(doc.schema_version, out);
      write_sized_string(doc.name, out);
      write_sized_string(doc.script, out);
      write_raw(doc.clear_color, out);

      OTHER_ASSERT(doc.objects.size() <= std::numeric_limits<uint32_t>::max(), "scene document too large");
      write_raw<uint32_t>(static_cast<uint32_t>(doc.objects.size()), out);
      {
        PROFILE_SECTION("write_scene_binary--objects");
        for (const object_record& record : doc.objects) {
          write_raw<natural_t>(record.file_id, out);
          write_raw<natural_t>(record.parent_file_id, out);
          write_sized_string(record.name, out);
          write_raw<uint8_t>(record.visible ? 1 : 0, out);

          OTHER_ASSERT(record.tags.size() <= std::numeric_limits<uint16_t>::max(), "too many tags on object '{}'", record.name);
          write_raw<uint16_t>(static_cast<uint16_t>(record.tags.size()), out);
          for (const std::string& tag : record.tags) {
            write_sized_string(tag, out);
          }

          OTHER_ASSERT(record.components.size() <= std::numeric_limits<uint16_t>::max(), "too many components on object '{}'", record.name);
          write_raw<uint16_t>(static_cast<uint16_t>(record.components.size()), out);
          for (const component_record& component : record.components) {
            write_raw<natural_t>(component.key_hash, out);
            OTHER_ASSERT(component.payload.size() <= std::numeric_limits<uint32_t>::max(), "component payload too large on object '{}'", record.name);
            write_raw<uint32_t>(static_cast<uint32_t>(component.payload.size()), out);
            out.insert(out.end(), component.payload.begin(), component.payload.end());
          }
        }
      }
      return out;
    }

    scene_parse_result parse_scene_binary(std::span<const uint8_t> bytes) {
      PROFILE_SECTION("parse_scene_binary");
      using namespace field_codec;
      size_t offset = 0;

      std::array<uint8_t, 4> magic = {};
      for (uint8_t& b : magic) {
        if (!read_raw(bytes, offset, b)) {
          return scene_parse_result::fail("truncated scene binary (magic)");
        }
      }
      if (magic != kSceneBinaryMagic) {
        return scene_parse_result::fail("not a scene binary (bad magic)");
      }

      uint16_t format_version = 0;
      uint16_t flags = 0;
      scene_document doc = {};
      if (!read_raw(bytes, offset, format_version) || !read_raw(bytes, offset, flags) || !read_raw(bytes, offset, doc.schema_version)) {
        return scene_parse_result::fail("truncated scene binary (header)");
      }
      if (format_version != kSceneBinaryFormatVersion) {
        return scene_parse_result::fail(std::format("unsupported scene binary format version {} (expected {})", format_version, kSceneBinaryFormatVersion));
      }

      if (!read_sized_string(bytes, offset, doc.name) || !read_sized_string(bytes, offset, doc.script) || !read_raw(bytes, offset, doc.clear_color)) {
        return scene_parse_result::fail("truncated scene binary (scene fields)");
      }

      uint32_t object_count = 0;
      if (!read_raw(bytes, offset, object_count)) {
        return scene_parse_result::fail("truncated scene binary (object count)");
      }

      ostd::vector<std::string> warnings = {};
      std::set<natural_t> seen_ids = {};
      {
        PROFILE_SECTION("parse_scene_binary--objects");
        for (uint32_t i = 0; i < object_count; ++i) {
          object_record record = {};
          uint8_t visible = 1;
          if (!read_raw(bytes, offset, record.file_id) || !read_raw(bytes, offset, record.parent_file_id) ||
              !read_sized_string(bytes, offset, record.name) || !read_raw(bytes, offset, visible)) {
            return scene_parse_result::fail(std::format("truncated scene binary (object {})", i));
          }
          record.visible = visible != 0;

          if (record.file_id == 0 || !seen_ids.insert(record.file_id).second) {
            return scene_parse_result::fail(std::format("scene binary object {} has invalid or duplicate id {:#x}", i, record.file_id));
          }
          if (record.parent_file_id != 0 && !seen_ids.contains(record.parent_file_id)) {
            return scene_parse_result::fail(std::format("scene binary object '{}' references parent {:#x} that does not precede it", record.name, record.parent_file_id));
          }

          uint16_t tag_count = 0;
          if (!read_raw(bytes, offset, tag_count)) {
            return scene_parse_result::fail(std::format("truncated scene binary (object '{}' tags)", record.name));
          }
          for (uint16_t t = 0; t < tag_count; ++t) {
            std::string tag = "";
            if (!read_sized_string(bytes, offset, tag)) {
              return scene_parse_result::fail(std::format("truncated scene binary (object '{}' tag {})", record.name, t));
            }
            record.tags.push_back(std::move(tag));
          }

          uint16_t component_count = 0;
          if (!read_raw(bytes, offset, component_count)) {
            return scene_parse_result::fail(std::format("truncated scene binary (object '{}' components)", record.name));
          }
          for (uint16_t c = 0; c < component_count; ++c) {
            component_record component = {};
            uint32_t payload_size = 0;
            if (!read_raw(bytes, offset, component.key_hash) || !read_raw(bytes, offset, payload_size) || offset + payload_size > bytes.size()) {
              return scene_parse_result::fail(std::format("truncated scene binary (object '{}' component {})", record.name, c));
            }
            component.payload.assign(bytes.begin() + offset, bytes.begin() + offset + payload_size);
            offset += payload_size;
            record.components.push_back(std::move(component));
          }

          doc.objects.push_back(std::move(record));
        }
      }

      if (offset != bytes.size()) {
        warnings.push_back(std::format("{} trailing bytes after scene binary payload", bytes.size() - offset));
      }
      return scene_parse_result::ok(std::move(doc), std::move(warnings));
    }

    /// -- toml --------------------------------------------------------------

    std::string write_scene_toml(const scene_document& doc) {
      PROFILE_SECTION("write_scene_toml");
      toml_writer w;
      w.comment("other environment scene document");
      w.table("scene");
      w.key("schema-version", doc.schema_version);
      w.key("name", doc.name);
      if (!doc.script.empty()) {
        w.key("script", doc.script);
      }
      const std::array<float, 4> clear_color = { doc.clear_color.r, doc.clear_color.g, doc.clear_color.b, doc.clear_color.a };
      w.key_array("clear-color", clear_color);

      {
        PROFILE_SECTION("write_scene_toml--objects");
        for (const object_record& record : doc.objects) {
          w.blank();
          w.table_array("objects");
          w.key("id", record.file_id);
          if (record.parent_file_id != 0) {
            w.key("parent", record.parent_file_id);
          }
          w.key("name", record.name);
          if (!record.visible) {
            w.key("visible", false);
          }
          if (!record.tags.empty()) {
            w.key_array("tags", record.tags);
          }

          for (const component_record& component : record.components) {
            const component_codec* codec = find_component_codec(component.key_hash);
            if (codec == nullptr) {
              w.comment(std::format("unknown component {:#018x} omitted", component.key_hash));
              continue;
            }
            const std::string table_path = std::format("objects.components.{}", codec->key);
            w.table(table_path);
            codec->payload_to_toml(component.payload, w, table_path);
          }
        }
      }
      return w.str();
    }

    scene_parse_result parse_scene_toml(std::string_view text) {
      PROFILE_SECTION("parse_scene_toml");
      toml::table root;
      {
        PROFILE_SECTION("parse_scene_toml--toml-parse");
        try {
          root = toml::parse(text);
        } catch (const toml::parse_error& e) {
          return scene_parse_result::fail(std::format("toml parse error: {}", std::string{ e.description() }));
        } catch (const std::exception& e) {
          return scene_parse_result::fail(std::format("toml parse error: {}", e.what()));
        }
      }

      const toml::table* scene_table = root.get_as<toml::table>("scene");
      if (scene_table == nullptr) {
        return scene_parse_result::fail("scene document missing [scene] table");
      }

      scene_document doc = {};
      ostd::vector<std::string> warnings = {};

      doc.schema_version = static_cast<uint32_t>(scene_table->get("schema-version") != nullptr ? scene_table->get("schema-version")->value_or<int64_t>(scene_document::kCurrentSchemaVersion) : scene_document::kCurrentSchemaVersion);
      if (doc.schema_version > scene_document::kCurrentSchemaVersion) {
        warnings.push_back(std::format("scene schema-version {} is newer than supported {}; unknown data will be skipped", doc.schema_version, scene_document::kCurrentSchemaVersion));
      }
      doc.name = scene_table->get("name") != nullptr ? scene_table->get("name")->value_or<std::string>("") : "";
      doc.script = scene_table->get("script") != nullptr ? scene_table->get("script")->value_or<std::string>("") : "";

      if (const toml::array* color = scene_table->get_as<toml::array>("clear-color"); color != nullptr && color->size() == 4) {
        for (size_t i = 0; i < 4; ++i) {
          doc.clear_color[static_cast<glm::length_t>(i)] = static_cast<float>(color->get(i)->value_or<double>(0.0));
        }
      }

      std::set<natural_t> seen_ids = {};
      natural_t next_synthetic_id = 1;
      if (const toml::array* objects = root.get_as<toml::array>("objects"); objects != nullptr) {
        PROFILE_SECTION("parse_scene_toml--objects");
        for (size_t i = 0; i < objects->size(); ++i) {
          const toml::table* object_table = objects->get(i)->as_table();
          if (object_table == nullptr) {
            return scene_parse_result::fail(std::format("[[objects]] entry {} is not a table", i));
          }

          object_record record = {};
          record.file_id = static_cast<natural_t>(object_table->get("id") != nullptr ? object_table->get("id")->value_or<int64_t>(0) : 0);
          if (record.file_id == 0) {
            /// ids are optional in hand-authored files; synthesize document-unique ones
            while (seen_ids.contains(next_synthetic_id)) {
              ++next_synthetic_id;
            }
            record.file_id = next_synthetic_id;
          }
          if (!seen_ids.insert(record.file_id).second) {
            return scene_parse_result::fail(std::format("[[objects]] entry {} has duplicate id {}", i, record.file_id));
          }

          record.parent_file_id = static_cast<natural_t>(object_table->get("parent") != nullptr ? object_table->get("parent")->value_or<int64_t>(0) : 0);
          if (record.parent_file_id != 0 && !seen_ids.contains(record.parent_file_id)) {
            return scene_parse_result::fail(std::format("[[objects]] entry '{}' references parent {} that does not precede it", record.name, record.parent_file_id));
          }

          record.name = object_table->get("name") != nullptr ? object_table->get("name")->value_or<std::string>("") : "";
          if (record.name.empty()) {
            return scene_parse_result::fail(std::format("[[objects]] entry {} is missing a name", i));
          }
          record.visible = object_table->get("visible") != nullptr ? object_table->get("visible")->value_or<bool>(true) : true;

          if (const toml::array* tags = object_table->get_as<toml::array>("tags"); tags != nullptr) {
            for (size_t t = 0; t < tags->size(); ++t) {
              if (const auto tag = tags->get(t)->value<std::string>(); tag.has_value()) {
                record.tags.push_back(*tag);
              } else {
                warnings.push_back(std::format("object '{}' has a non-string tag; ignored", record.name));
              }
            }
          }

          if (const toml::table* components = object_table->get_as<toml::table>("components"); components != nullptr) {
            /// iterate codecs in registration order for stable payload order, then flag unknowns
            for (const component_codec& codec : component_codecs()) {
              const toml::table* component_table = components->get_as<toml::table>(codec.key);
              if (component_table == nullptr) {
                continue;
              }
              record.components.push_back(component_record{
                .key_hash = codec.key_hash,
                .payload = codec.payload_from_toml(*component_table, warnings),
              });
            }
            for (const auto& [key, node] : *components) {
              if (find_component_codec(std::string_view{ key.str() }) == nullptr) {
                warnings.push_back(std::format("object '{}' has unknown component table '{}'; ignored", record.name, std::string{ key.str() }));
              }
            }
          }

          doc.objects.push_back(std::move(record));
        }
      }

      return scene_parse_result::ok(std::move(doc), std::move(warnings));
    }

    /// -- files -------------------------------------------------------------

    scene_parse_result load_scene_document(const filepath& path) {
      PROFILE_SECTION("load_scene_document");
      std::ifstream in(path, std::ios::binary);
      if (!in.is_open()) {
        return scene_parse_result::fail(std::format("failed to open scene file '{}'", path.string()));
      }
      ostd::vector<char> contents{ std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>() };

      const std::string extension = path.extension().string();
      if (extension == kSceneTomlExtension) {
        return parse_scene_toml(std::string_view{ contents.data(), contents.size() });
      }
      if (extension == kSceneBinaryExtension) {
        return parse_scene_binary(std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(contents.data()), contents.size()));
      }
      return scene_parse_result::fail(std::format("'{}' is not a scene document ({}/{} expected)", path.string(), kSceneTomlExtension, kSceneBinaryExtension));
    }

    bool save_scene_document(const scene_document& doc, const filepath& path) {
      PROFILE_SECTION("save_scene_document");
      const std::string extension = path.extension().string();
      std::ofstream out(path, std::ios::binary | std::ios::trunc);
      if (!out.is_open()) {
        CORE_LOG_ERROR("failed to open '{}' for writing", path.string());
        return false;
      }

      if (extension == kSceneTomlExtension) {
        const std::string text = write_scene_toml(doc);
        out.write(text.data(), static_cast<std::streamsize>(text.size()));
        return out.good();
      }
      if (extension == kSceneBinaryExtension) {
        const ostd::vector<uint8_t> bytes = write_scene_binary(doc);
        out.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
        return out.good();
      }

      CORE_LOG_ERROR("'{}' is not a scene document extension ({}/{} expected)", path.string(), kSceneTomlExtension, kSceneBinaryExtension);
      return false;
    }

    ostd::vector<component_asset_ref> collect_scene_asset_refs(const scene_document& doc) {
      PROFILE_SECTION("collect_scene_asset_refs");
      ostd::vector<component_asset_ref> refs = {};
      for (const object_record& object : doc.objects) {
        for (const component_record& component : object.components) {
          const component_codec* codec = find_component_codec(component.key_hash);
          if (codec != nullptr && codec->collect_asset_refs != nullptr) {
            codec->collect_asset_refs(component.payload, refs);
          }
        }
      }
      return refs;
    }

  }  // namespace serialization
}  // namespace other

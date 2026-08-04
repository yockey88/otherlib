/**
 * \file serialization/component_codec.cpp
 **/
#include "serialization/component_codec.hpp"

#include <algorithm>

#include "script/script_object.hpp"
#include "script/scripting_environment.hpp"

#include "object/animation_component.hpp"
#include "object/audio_listener_component.hpp"
#include "object/audio_source_component.hpp"
#include "object/camera_component.hpp"
#include "object/grid_component.hpp"
#include "object/light_component.hpp"
#include "object/physics_component.hpp"
#include "object/render_component.hpp"
#include "object/script_component.hpp"
#include "object/transform.hpp"

namespace other {
  namespace serialization {
    namespace {

      constexpr std::string_view kBehaviorsFieldName = "behaviors";

      ostd::vector<std::string> decode_behavior_names(std::span<const uint8_t> payload) {
        ostd::map<natural_t, detail::payload_field> fields = {};
        if (!detail::index_payload(payload, fields)) {
          CORE_LOG_ERROR("truncated script component payload");
          return {};
        }

        ostd::vector<std::string> names = {};
        const auto it = fields.find(FNV(std::string{ kBehaviorsFieldName }));
        if (it != fields.end()) {
          (void)field_codec::decode_body(it->second.tag, it->second.body, names);
        }
        return names;
      }

      ostd::vector<uint8_t> encode_behavior_names(const ostd::vector<std::string>& names) {
        ostd::vector<uint8_t> payload = {};
        if (names.empty()) {
          return payload;  /// empty payload -> implicit component record is omitted
        }
        field_codec::write_raw<natural_t>(FNV(std::string{ kBehaviorsFieldName }), payload);
        field_codec::encode_value(names, payload);
        return payload;
      }

      /// script_component's persistent state is the list of attached behavior type
      /// names, which lives in the scripting environment's script_object rather than
      /// the component struct — a custom codec with the shared payload discipline
      component_codec make_script_codec() {
        component_codec codec = {
          .key = "script",
          .display_name = "Script",
          .key_hash = FNV(std::string{ "script" }),
          .implicit = true,
        };

        codec.has = [](scene& s, scene_object* object) -> bool {
          return s.has_component<script_component>(object);
        };

        codec.capture = [](scene& s, scene_object* object, const codec_services&) -> ostd::vector<uint8_t> {
          const script_component* component = s.get_component<script_component>(object);
          OTHER_ASSERT(component != nullptr, "script capture called for an object without a script component");
          if (component->script_object_id < 0) {
            return {};
          }

          auto* script_env = subsystem<scripting_environment>::get();
          OTHER_ASSERT(script_env != nullptr, "scripting environment unavailable during scene capture");
          script_object* script_obj = script_env->get_object(component->script_object_id);
          if (script_obj == nullptr) {
            return {};
          }

          ostd::vector<std::string> names = {};
          for (const auto& handle : script_obj->behavior_handles) {
            names.push_back(handle.type_name);
          }
          return encode_behavior_names(names);
        };

        codec.apply = [](scene& s, scene_object* object, std::span<const uint8_t> payload, const codec_services&) {
          script_component* component = s.get_component<script_component>(object);
          OTHER_ASSERT(component != nullptr, "script apply called for an object without a script component");
          const ostd::vector<std::string> names = decode_behavior_names(payload);
          for (const std::string& name : names) {
            /// attach is idempotent,  behaviors surviving a play-stop restore are kept as-is
            component->add_behavior(name);
          }

          /// reconcile: behaviors on the live object that the document does not list are
          ///  genuine removals (e.g. added during play, rolled back by stop's restore).
          ///  objects whose captured behavior list was empty emit no script record at
          ///  all, so play-added behaviors on those objects escape this prune
          auto* script_env = subsystem<scripting_environment>::get();
          if (script_env == nullptr || component->script_object_id < 0) {
            return;
          }
          script_object* script_obj = script_env->get_object(component->script_object_id);
          if (script_obj == nullptr) {
            return;
          }
          ostd::vector<std::string> extras = {};
          for (const auto& handle : script_obj->behavior_handles) {
            if (std::find(names.begin(), names.end(), handle.type_name) == names.end()) {
              extras.push_back(handle.type_name);
            }
          }
          for (const std::string& extra : extras) {
            component->remove_behavior(extra);
          }
        };

        codec.payload_to_toml = [](std::span<const uint8_t> payload, toml_writer& w, const std::string&) {
          const ostd::vector<std::string> names = decode_behavior_names(payload);
          if (!names.empty()) {
            w.key_array(kBehaviorsFieldName, names);
          }
        };

        codec.payload_from_toml = [](const toml::table& table, ostd::vector<std::string>& warnings) -> ostd::vector<uint8_t> {
          ostd::vector<std::string> names = {};
          if (const toml::node* node = table.get(kBehaviorsFieldName); node != nullptr) {
            if (!detail::read_toml_value(*node, names, warnings)) {
              warnings.push_back("script component 'behaviors' must be an array of strings");
            }
          }
          return encode_behavior_names(names);
        };

        return codec;
      }

      /// authored camera tables usually give only position/direction; the view matrix
      ///  reads the basis, so canonicalize it (with look()'s degenerate-up fallback)
      ///  after the generic field apply
      component_codec make_camera_codec() {
        component_codec codec = make_generic_codec<camera_component>("camera", "Camera", /*implicit=*/false);
        auto generic_apply = codec.apply;
        codec.apply = [generic_apply](scene& s, scene_object* object, std::span<const uint8_t> payload, const codec_services& services) {
          generic_apply(s, object, payload, services);
          camera_component* component = s.get_component<camera_component>(object);
          OTHER_ASSERT(component != nullptr, "camera apply left the object without a camera component");
          component->camera.look(component->camera.position, component->camera.position + component->camera.direction);
        };
        return codec;
      }

      ostd::vector<component_codec> build_builtin_codecs() {
        ostd::vector<component_codec> codecs = {};
        codecs.push_back(make_generic_codec<transform>("transform", "Transform", /*implicit=*/true));
        codecs.push_back(make_script_codec());
        codecs.push_back(make_generic_codec<render_component>("render", "Graphics Object", /*implicit=*/false));
        codecs.push_back(make_generic_codec<physics_component>("physics", "Physics Object", /*implicit=*/false));
        codecs.push_back(make_camera_codec());
        codecs.push_back(make_generic_codec<grid_component>("grid", "Grid", /*implicit=*/false));
        codecs.push_back(make_generic_codec<point_light_component>("point-light", "Point Light", /*implicit=*/false));
        codecs.push_back(make_generic_codec<direction_light_component>("direction-light", "Directional Light", /*implicit=*/false));
        codecs.push_back(make_generic_codec<animation_component>("animation", "Animation", /*implicit=*/false));
        codecs.push_back(make_generic_codec<audio_source_component>("audio-source", "Audio Source", /*implicit=*/false));
        codecs.push_back(make_generic_codec<audio_listener_component>("audio-listener", "Audio Listener", /*implicit=*/false));

        for (size_t i = 0; i < codecs.size(); ++i) {
          for (size_t j = i + 1; j < codecs.size(); ++j) {
            OTHER_ASSERT(codecs[i].key_hash != codecs[j].key_hash, "component codec key collision: '{}' vs '{}'", codecs[i].key, codecs[j].key);
          }
        }
        return codecs;
      }

    }  // namespace

    const ostd::vector<component_codec>& component_codecs() {
      static const ostd::vector<component_codec> codecs = build_builtin_codecs();
      return codecs;
    }

    const component_codec* find_component_codec(natural_t key_hash) {
      for (const component_codec& codec : component_codecs()) {
        if (codec.key_hash == key_hash) {
          return &codec;
        }
      }
      return nullptr;
    }

    const component_codec* find_component_codec(std::string_view key) {
      for (const component_codec& codec : component_codecs()) {
        if (codec.key == key) {
          return &codec;
        }
      }
      return nullptr;
    }

    codec_services& default_codec_services() {
      static codec_services services = {};
      return services;
    }

  }  // namespace serialization
}  // namespace other

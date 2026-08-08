/**
 * \file serialization/component_codec.hpp
 *
 * per-component-type conversion table (component <-> payload <-> .oscn toml) for all scene persistence;
 * document-level conversions are engine-free; asset_identifier_field members serialize as portable paths
 */
#ifndef OTHER_SCENE_SERIALIZATION_COMPONENT_CODEC_HPP
#define OTHER_SCENE_SERIALIZATION_COMPONENT_CODEC_HPP

#include <functional>

#include <toml++/toml.h>

#include "core/fnv.hpp"
#include "serialization/toml_writer.hpp"

#include "asset/asset.hpp"
#include "scene/scene.hpp"
#include "serialization/scene_document.hpp"
#include "serialization/scene_field_codec.hpp"

namespace other {
  namespace serialization {

    struct codec_services {
      /// runtime asset id -> portable path (virtual path when the asset has one)
      std::function<std::string(natural_t asset_id)> asset_path_of = nullptr;
      /// portable path -> runtime asset id, queueing the load (0 = unresolved)
      std::function<natural_t(const std::string& path)> resolve_asset = nullptr;
    };

    /// a portable asset reference decoded out of a component payload, as stored —
    /// resolution against the working directory / mounts is the caller's concern
    struct component_asset_ref {
      std::string path = "";
      asset::type type = asset::EMPTY;
    };

    struct component_codec {
      std::string_view key = "";           /// toml table key + binary hash source
      std::string_view display_name = "";  /// matches scene_system's component_registry name
      natural_t key_hash = 0;              /// FNV(key)
      /// implicit components exist on every object (transform, script): apply edits them
      /// in place and capture may return an empty payload to omit the record entirely
      bool implicit = false;

      std::function<bool(scene&, scene_object*)> has = nullptr;
      std::function<ostd::vector<uint8_t>(scene&, scene_object*, const codec_services&)> capture = nullptr;
      std::function<void(scene&, scene_object*, std::span<const uint8_t>, const codec_services&)> apply = nullptr;

      /// document-level, engine-free (cli safe); table_path is the dotted path of the
      /// component's own table (e.g. "objects.components.camera") for nested sub-tables
      std::function<void(std::span<const uint8_t>, toml_writer&, const std::string&)> payload_to_toml = nullptr;
      std::function<ostd::vector<uint8_t>(const toml::table&, ostd::vector<std::string>&)> payload_from_toml = nullptr;

      /// document-level, engine-free: append every non-empty asset path the payload
      /// carries (manifest parsing); null for codecs without asset reference members
      std::function<void(std::span<const uint8_t>, ostd::vector<component_asset_ref>&)> collect_asset_refs = nullptr;
    };

    /// registration order = document emission order; self-populates with the builtin
    /// component codecs on first use, so no engine boot is required
    const ostd::vector<component_codec>& component_codecs();
    const component_codec* find_component_codec(natural_t key_hash);
    const component_codec* find_component_codec(std::string_view key);

    /// process-wide services for live-scene capture/apply, wired by the asset_system at boot;
    /// tests/cli leave them null (asset refs then serialize as empty paths / resolve to 0)
    codec_services& default_codec_services();

    /// generic codec machinery (used by component_codec.cpp and tests)
    namespace detail {

      template <typename member_descriptor_t>
      consteval bool is_asset_reference_member() {
        return refl::descriptor::has_attribute<attr::asset_identifier_field>(member_descriptor_t{});
      }

      /// -- toml emission per member value ---------------------------------

      template <typename T>
      void emit_toml_value(toml_writer& w, std::string_view key, const T& value) {
        using no_cvref_t = std::remove_cvref_t<T>;
        if constexpr (std::is_same_v<no_cvref_t, bool>) {
          w.key(key, value);
        } else if constexpr (std::is_enum_v<no_cvref_t>) {
          w.key(key, static_cast<std::underlying_type_t<no_cvref_t>>(value));
        } else if constexpr (std::is_arithmetic_v<no_cvref_t>) {
          w.key(key, value);
        } else if constexpr (is_stringlike_type<no_cvref_t>) {
          w.key(key, std::string_view{ value });
        } else if constexpr (std::is_same_v<no_cvref_t, glm::quat>) {
          const std::array<float, 4> wxyz = { value.w, value.x, value.y, value.z };
          w.key_array(key, wxyz);
        } else if constexpr (field_codec::scene_glm_value<no_cvref_t>) {
          constexpr size_t component_count = sizeof(no_cvref_t) / sizeof(float);
          w.key_array(key, std::span<const float>(glm::value_ptr(value), component_count));
        } else if constexpr (field_codec::scene_container_value<no_cvref_t>) {
          using element_t = typename no_cvref_t::value_type;
          if constexpr (field_codec::scene_reflected_value<element_t>) {
            w.key_inline_table_array(key, std::ranges::size(value), [&](toml_writer::inline_table& element, size_t index) {
              refl::util::for_each(refl::reflect<element_t>().members, [&](auto member) {
                using member_descriptor_t = std::decay_t<decltype(member)>;
                if constexpr (other::detail::should_serialize_member<member_descriptor_t>()) {
                  using member_t = std::remove_cvref_t<decltype(member(std::declval<const element_t&>()))>;
                  static_assert(std::is_arithmetic_v<member_t> || std::is_enum_v<member_t> || is_stringlike_type<member_t> || field_codec::scene_container_value<member_t>,
                                "inline table elements support scalar / string / scalar-array members only");
                  const auto& element_value = value[index];
                  if constexpr (field_codec::scene_container_value<member_t>) {
                    element.key_array(std::string{ member.name }, member(element_value));
                  } else if constexpr (std::is_enum_v<member_t>) {
                    element.key(std::string{ member.name }, static_cast<std::underlying_type_t<member_t>>(member(element_value)));
                  } else {
                    element.key(std::string{ member.name }, member(element_value));
                  }
                }
              });
            });
          } else {
            w.key_array(key, value);
          }
        } else {
          static_assert(field_codec::scene_reflected_value<no_cvref_t>, "unsupported toml member type");
          /// handled by the caller's nested-table pass
        }
      }

      template <typename T>
      constexpr bool is_nested_table_member =
        field_codec::scene_reflected_value<std::remove_cvref_t<T>> &&
        !field_codec::scene_glm_value<std::remove_cvref_t<T>> &&
        !std::is_arithmetic_v<std::remove_cvref_t<T>> &&
        !is_stringlike_type<std::remove_cvref_t<T>> &&
        !field_codec::scene_container_value<std::remove_cvref_t<T>>;

      /// emits all serializable members of @p value into the writer's current table: scalars first
      /// (toml forbids keys after a sub-table opens), then nested members as sub-tables; asset refs are skipped (emitted separately as paths)
      template <typename T>
      void emit_toml_members(toml_writer& w, const std::string& table_path, const T& value) {
        refl::util::for_each(refl::reflect<T>().members, [&](auto member) {
          using member_descriptor_t = std::decay_t<decltype(member)>;
          if constexpr (other::detail::should_serialize_member<member_descriptor_t>() && !is_asset_reference_member<member_descriptor_t>()) {
            using member_t = std::remove_cvref_t<decltype(member(value))>;
            if constexpr (!is_nested_table_member<member_t>) {
              emit_toml_value(w, std::string{ member.name }, member(value));
            }
          }
        });
        refl::util::for_each(refl::reflect<T>().members, [&](auto member) {
          using member_descriptor_t = std::decay_t<decltype(member)>;
          if constexpr (other::detail::should_serialize_member<member_descriptor_t>() && !is_asset_reference_member<member_descriptor_t>()) {
            using member_t = std::remove_cvref_t<decltype(member(value))>;
            if constexpr (is_nested_table_member<member_t>) {
              const std::string nested_path = table_path + "." + std::string{ member.name };
              w.table(nested_path);
              emit_toml_members(w, nested_path, member(value));
            }
          }
        });
      }

      /// -- toml reading per member value -----------------------------------

      template <typename T>
      bool read_toml_value(const toml::node& node, T& out, ostd::vector<std::string>& warnings);

      template <typename T>
      bool read_toml_members(const toml::table& table, T& out, ostd::vector<std::string>& warnings) {
        refl::util::for_each(refl::reflect<T>().members, [&](auto member) {
          using member_descriptor_t = std::decay_t<decltype(member)>;
          if constexpr (other::detail::should_serialize_member<member_descriptor_t>()) {
            const std::string key{ member.name };
            const toml::node* node = table.get(key);
            if (node == nullptr) {
              return;  /// missing member keeps its default
            }
            if (!read_toml_value(*node, member(out), warnings)) {
              warnings.push_back(std::format("toml key '{}' of '{}' has an unexpected type and was ignored", key, get_type_name_safe<T>()));
            }
          }
        });
        return true;
      }

      template <typename T>
      bool read_toml_value(const toml::node& node, T& out, ostd::vector<std::string>& warnings) {
        using no_cvref_t = std::remove_cvref_t<T>;
        if constexpr (std::is_same_v<no_cvref_t, bool>) {
          const auto v = node.value<bool>();
          if (!v.has_value()) {
            return false;
          }
          out = *v;
          return true;
        } else if constexpr (std::is_enum_v<no_cvref_t>) {
          const auto v = node.value<int64_t>();
          if (!v.has_value()) {
            return false;
          }
          out = static_cast<no_cvref_t>(*v);
          return true;
        } else if constexpr (std::is_integral_v<no_cvref_t>) {
          const auto v = node.value<int64_t>();
          if (!v.has_value()) {
            return false;
          }
          out = static_cast<no_cvref_t>(*v);
          return true;
        } else if constexpr (std::is_floating_point_v<no_cvref_t>) {
          /// accept integers where floats are expected ("scale = 1")
          if (const auto f = node.value<double>(); f.has_value()) {
            out = static_cast<no_cvref_t>(*f);
            return true;
          }
          return false;
        } else if constexpr (is_stringlike_type<no_cvref_t>) {
          const auto v = node.value<std::string>();
          if (!v.has_value()) {
            return false;
          }
          out = *v;
          return true;
        } else if constexpr (std::is_same_v<no_cvref_t, glm::quat>) {
          const toml::array* arr = node.as_array();
          if (arr == nullptr || arr->size() != 4) {
            return false;
          }
          std::array<float, 4> wxyz = {};
          for (size_t i = 0; i < 4; ++i) {
            const auto v = arr->get(i)->value<double>();
            if (!v.has_value()) {
              return false;
            }
            wxyz[i] = static_cast<float>(*v);
          }
          out = glm::quat(wxyz[0], wxyz[1], wxyz[2], wxyz[3]);
          return true;
        } else if constexpr (field_codec::scene_glm_value<no_cvref_t>) {
          constexpr size_t component_count = sizeof(no_cvref_t) / sizeof(float);
          const toml::array* arr = node.as_array();
          if (arr == nullptr || arr->size() != component_count) {
            return false;
          }
          float* components = glm::value_ptr(out);
          for (size_t i = 0; i < component_count; ++i) {
            const auto v = arr->get(i)->value<double>();
            if (!v.has_value()) {
              return false;
            }
            components[i] = static_cast<float>(*v);
          }
          return true;
        } else if constexpr (field_codec::scene_container_value<no_cvref_t>) {
          using element_t = typename no_cvref_t::value_type;
          const toml::array* arr = node.as_array();
          if (arr == nullptr) {
            return false;
          }
          out.clear();
          bool all_elements_ok = true;
          arr->for_each([&](auto&& element_node) {
            element_t element{};
            if constexpr (field_codec::scene_reflected_value<element_t>) {
              const toml::table* element_table = element_node.as_table();
              if (element_table == nullptr || !read_toml_members(*element_table, element, warnings)) {
                all_elements_ok = false;
                return;
              }
            } else {
              if (!read_toml_value(element_node, element, warnings)) {
                all_elements_ok = false;
                return;
              }
            }
            out.push_back(std::move(element));
          });
          return all_elements_ok;
        } else {
          static_assert(field_codec::scene_reflected_value<no_cvref_t>, "unsupported toml member type");
          const toml::table* sub_table = node.as_table();
          if (sub_table == nullptr) {
            return false;
          }
          return read_toml_members(*sub_table, out, warnings);
        }
      }

      /// -- payload <-> member map -------------------------------------------

      struct payload_field {
        uint8_t tag = 0;
        std::span<const uint8_t> body = {};
      };

      /// index a payload's fields by id; false = truncated payload
      inline bool index_payload(std::span<const uint8_t> payload, ostd::map<natural_t, payload_field>& out) {
        size_t offset = 0;
        while (offset < payload.size()) {
          natural_t field_id = 0;
          uint8_t tag = 0;
          std::span<const uint8_t> body = {};
          if (!field_codec::read_raw(payload, offset, field_id) || !field_codec::read_frame(payload, offset, tag, body)) {
            return false;
          }
          out[field_id] = payload_field{ .tag = tag, .body = body };
        }
        return true;
      }

    }  // namespace detail

    /// generic reflection-driven codec for a component type T
    template <typename T>
    component_codec make_generic_codec(std::string_view key, std::string_view display_name, bool implicit) {
      component_codec codec = {
        .key = key,
        .display_name = display_name,
        .key_hash = FNV(std::string{ key }),
        .implicit = implicit,
      };

      codec.has = [](scene& s, scene_object* object) -> bool {
        return s.has_component<T>(object);
      };

      codec.capture = [](scene& s, scene_object* object, const codec_services& services) -> ostd::vector<uint8_t> {
        const T* component = s.get_component<T>(object);
        OTHER_ASSERT(component != nullptr, "capture called for a component the object does not have");

        ostd::vector<uint8_t> payload = {};
        refl::util::for_each(refl::reflect<T>().members, [&](auto member) {
          using member_descriptor_t = std::decay_t<decltype(member)>;
          if constexpr (other::detail::should_serialize_member<member_descriptor_t>()) {
            using member_t = std::remove_cvref_t<decltype(member(*component))>;
            field_codec::write_raw<natural_t>(FNV(std::string{ member.name }), payload);
            if constexpr (detail::is_asset_reference_member<member_descriptor_t>()) {
              static_assert(std::is_same_v<member_t, natural_t>, "asset reference members must be natural_t ids");
              std::string path = "";
              if (member(*component) != 0 && services.asset_path_of != nullptr) {
                path = services.asset_path_of(member(*component));
              }
              field_codec::encode_value(path, payload);
            } else {
              field_codec::encode_value(member(*component), payload);
            }
          }
        });
        return payload;
      };

      codec.apply = [](scene& s, scene_object* object, std::span<const uint8_t> payload, const codec_services& services) {
        auto decode_into = [&](T& target) {
          ostd::map<natural_t, detail::payload_field> fields = {};
          if (!detail::index_payload(payload, fields)) {
            CORE_LOG_ERROR("truncated payload for component '{}' on object '{}'", get_type_name_safe<T>(), object->name);
            return;
          }
          refl::util::for_each(refl::reflect<T>().members, [&](auto member) {
            using member_descriptor_t = std::decay_t<decltype(member)>;
            if constexpr (other::detail::should_serialize_member<member_descriptor_t>()) {
              const auto it = fields.find(FNV(std::string{ member.name }));
              if (it == fields.end()) {
                return;
              }
              if constexpr (detail::is_asset_reference_member<member_descriptor_t>()) {
                std::string path = "";
                if (field_codec::decode_body(it->second.tag, it->second.body, path) && !path.empty() && services.resolve_asset != nullptr) {
                  member(target) = services.resolve_asset(path);
                }
              } else {
                if (!field_codec::decode_body(it->second.tag, it->second.body, member(target))) {
                  CORE_LOG_WARN("component '{}' member '{}' had an unexpected encoding and was skipped", get_type_name_safe<T>(), std::string{ member.name });
                }
              }
            }
          });
        };

        if (s.has_component<T>(object)) {
          decode_into(*s.get_component<T>(object));
        } else {
          T value{};
          decode_into(value);
          s.add_component<T>(object, std::move(value));
        }
      };

      codec.payload_to_toml = [](std::span<const uint8_t> payload, toml_writer& w, const std::string& table_path) {
        ostd::map<natural_t, detail::payload_field> fields = {};
        if (!detail::index_payload(payload, fields)) {
          return;
        }
        /// materialize a T from the payload for non-asset members so emission reuses
        /// the reflected member walk; asset members carry strings and emit directly
        T value{};
        ostd::vector<std::string> ignored = {};
        refl::util::for_each(refl::reflect<T>().members, [&](auto member) {
          using member_descriptor_t = std::decay_t<decltype(member)>;
          if constexpr (other::detail::should_serialize_member<member_descriptor_t>() && !detail::is_asset_reference_member<member_descriptor_t>()) {
            const auto it = fields.find(FNV(std::string{ member.name }));
            if (it != fields.end()) {
              (void)field_codec::decode_body(it->second.tag, it->second.body, member(value));
            }
          }
        });

        /// pass 1a: asset path strings
        refl::util::for_each(refl::reflect<T>().members, [&](auto member) {
          using member_descriptor_t = std::decay_t<decltype(member)>;
          if constexpr (other::detail::should_serialize_member<member_descriptor_t>() && detail::is_asset_reference_member<member_descriptor_t>()) {
            const auto it = fields.find(FNV(std::string{ member.name }));
            std::string path = "";
            if (it != fields.end()) {
              (void)field_codec::decode_body(it->second.tag, it->second.body, path);
            }
            if (!path.empty()) {
              w.key(std::string{ member.name }, path);
            }
          }
        });
        /// pass 1b scalars + pass 2 nested tables (asset members excluded above)
        detail::emit_toml_members<T>(w, table_path, value);
      };

      codec.collect_asset_refs = [](std::span<const uint8_t> payload, ostd::vector<component_asset_ref>& out) {
        ostd::map<natural_t, detail::payload_field> fields = {};
        if (!detail::index_payload(payload, fields)) {
          return;
        }
        refl::util::for_each(refl::reflect<T>().members, [&](auto member) {
          using member_descriptor_t = std::decay_t<decltype(member)>;
          if constexpr (other::detail::should_serialize_member<member_descriptor_t>() && detail::is_asset_reference_member<member_descriptor_t>()) {
            const auto it = fields.find(FNV(std::string{ member.name }));
            if (it == fields.end()) {
              return;
            }
            std::string path = "";
            if (field_codec::decode_body(it->second.tag, it->second.body, path) && !path.empty()) {
              out.push_back({
                .path = std::move(path),
                .type = refl::descriptor::get_attribute<attr::asset_identifier_field>(member_descriptor_t{}).asset_type,
              });
            }
          }
        });
      };

      codec.payload_from_toml = [](const toml::table& table, ostd::vector<std::string>& warnings) -> ostd::vector<uint8_t> {
        ostd::vector<uint8_t> payload = {};
        T defaults{};
        refl::util::for_each(refl::reflect<T>().members, [&](auto member) {
          using member_descriptor_t = std::decay_t<decltype(member)>;
          if constexpr (other::detail::should_serialize_member<member_descriptor_t>()) {
            const std::string key{ member.name };
            if constexpr (detail::is_asset_reference_member<member_descriptor_t>()) {
              std::string path = "";
              if (const toml::node* node = table.get(key); node != nullptr) {
                if (const auto v = node->value<std::string>(); v.has_value()) {
                  path = *v;
                }
              }
              field_codec::write_raw<natural_t>(FNV(key), payload);
              field_codec::encode_value(path, payload);
            } else {
              auto member_value = member(defaults);
              if (const toml::node* node = table.get(key); node != nullptr) {
                if (!detail::read_toml_value(*node, member_value, warnings)) {
                  warnings.push_back(std::format("toml key '{}' of '{}' has an unexpected type and was ignored", key, get_type_name_safe<T>()));
                }
              }
              field_codec::write_raw<natural_t>(FNV(key), payload);
              field_codec::encode_value(member_value, payload);
            }
          }
        });
        return payload;
      };

      return codec;
    }

  }  // namespace serialization
}  // namespace other

#endif  // OTHER_SCENE_SERIALIZATION_COMPONENT_CODEC_HPP

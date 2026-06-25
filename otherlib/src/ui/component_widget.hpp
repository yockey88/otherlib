/**
 * \file ui/component_widget.hpp
 **/
#ifndef OTHERLIB_UI_COMPONENT_WIDGET_HPP
#define OTHERLIB_UI_COMPONENT_WIDGET_HPP

#include <cstdint>
#include <string>
#include <string_view>

#include "math/orthonormal_basis.hpp"
#include "serialization/reflection.hpp"

#include "renderer/ui/colors.hpp"
#include "script/script_object.hpp"

#include "object/scene_object.hpp"
#include "scene/scene.hpp"

#include "driver/driver.hpp"
#include "driver/systems/asset_system.hpp"
#include "ui/inspector_widgets.hpp"

#include "asset/asset.hpp"
#include "asset/asset_handler.hpp"

namespace other {

  template <typename T>
  struct property_ui;

  namespace ui {

    namespace detail {

      template <typename FT>
      concept has_property_ui = requires {
        property_ui<FT>{}(std::declval<const std::string&>(), std::declval<FT&>(), std::declval<scene*>(), std::declval<scene_object*>());
      };

      inline bool is_asset_id_field(const std::string& field_name) {
        return field_name.find("asset_id") != std::string::npos;
      }

      inline std::string asset_display_name_for_id(natural_t asset_id, const asset_handler* handler) {
        if (asset_id == 0 || handler == nullptr) {
          return "None";
        }

        const asset* a = handler->get_loaded_asset(asset_id);
        if (a != nullptr) {
          return a->load_path.filename().string();
        }

        asset_state state = handler->get_asset_state(asset_id);
        if (state == asset_state::LOADING) {
          return "Loading...";
        }

        return std::format("Asset #{}", asset_id);
      }

      template <typename FT>
      // clang-format off
      bool draw_inspector_field(const std::string& field_name, FT& field_value, asset::type asset_type, 
                                scene* active_scene, scene_object* object, asset_handler* handler = nullptr, driver* drvr = nullptr, 
                                std::string display_name = "") {
        // clang-format on
        OTHER_ASSERT(drvr != nullptr, "Driver pointer is null in draw_inspector_field for field '{}'", field_name);
        bool changed = false;

        if (display_name.empty()) {
          display_name = field_name;
        }
        if constexpr (std::same_as<FT, natural_t> || std::same_as<FT, integer_t>) {
          if (asset_type != asset::type::EMPTY) {
            std::string asset_name = asset_display_name_for_id(field_value, handler);
            opt<natural_t> dropped_id = inspector::property_asset_slot(display_name, field_value, asset_name, asset_type);

            auto& assets = drvr->get_kernel().get_core_system<asset_system>();
            auto& asset_manager = assets.get_asset_manager();

            bool asset_dropped = dropped_id.has_value() && dropped_id.value() != field_value;
            bool asset_exists = asset_dropped && asset_manager->asset_exists(*dropped_id);
            if (asset_exists) {
              field_value = *dropped_id;
              changed = true;
            } else if (asset_dropped) {
              CORE_LOG_ERROR("Dropped asset ID {} does not exist in asset manager for field '{}'", *dropped_id, field_name);
            }
          } else {
            if constexpr (std::is_same_v<FT, natural_t>) {
              natural_t ival = field_value;
              changed = inspector::property_uint64(display_name, ival);
            } else if constexpr (std::is_same_v<FT, integer_t>) {
              integer_t ival = field_value;
              changed = inspector::property_int64(display_name, ival);
            }
          }
        } else if constexpr (has_property_ui<FT>) {
          changed = property_ui<FT>{}(field_name, field_value, active_scene, object);
        } else if constexpr (std::is_same_v<FT, glm::vec3>) {
          changed = inspector::property_vec3(field_name, field_value);
        } else if constexpr (std::is_same_v<FT, glm::vec2>) {
          changed = inspector::property_vec2(field_name, field_value);
        } else if constexpr (std::is_same_v<FT, glm::vec4>) {
          changed = inspector::property_vec4(field_name, field_value);
        } else if constexpr (std::is_same_v<FT, glm::quat>) {
          /// render quat as euler angles (yaw/pitch/roll) in the inspector
          glm::vec3 euler = glm::degrees(glm::eulerAngles(field_value));
          if (inspector::property_vec3(field_name, euler, 1.f)) {
            field_value = glm::quat(glm::radians(euler));
            changed = true;
          }
        } else if constexpr (std::is_same_v<FT, orthonormal_basis>) {
        } else if constexpr (reflected_type<FT>) {
          for_each(refl::reflect(field_value).members, [&](auto sub_field) {
            std::string sub_name = std::string{ sub_field.name };

            asset::type sub_asset_type = asset::type::EMPTY;
            if constexpr (refl::descriptor::has_attribute<attr::asset_identifier_field>(sub_field)) {
              auto& asset_id_attr = refl::descriptor::get_attribute<attr::asset_identifier_field>(sub_field);
              asset_type = asset_id_attr.asset_type;
            }

            changed |= draw_inspector_field(sub_name, sub_field(field_value), sub_asset_type, active_scene, object, handler, drvr);
          });
        } else if constexpr (std::is_same_v<FT, bool>) {
          changed = inspector::property_bool(field_name, field_value);
        } else if constexpr (std::is_same_v<FT, float>) {
          changed = inspector::property_float(field_name, field_value);
        } else if constexpr (std::is_same_v<FT, double>) {
          float fval = static_cast<float>(field_value);
          if (inspector::property_float(field_name, fval)) {
            field_value = static_cast<double>(fval);
            changed = true;
          }
        } else if constexpr (std::is_integral_v<FT> && !std::is_same_v<FT, bool>) {
          if (sizeof(FT) <= 4) {
            int32_t ival = static_cast<int32_t>(field_value);
            if (inspector::property_int32(field_name, ival)) {
              field_value = static_cast<FT>(ival);
              changed = true;
            }
          } else {
            int64_t ival = static_cast<int64_t>(field_value);
            if (inspector::property_int64(field_name, ival)) {
              field_value = static_cast<FT>(ival);
              changed = true;
            }
          }
        } else if constexpr (std::is_same_v<FT, std::string>) {
          char buf[256];
          std::strncpy(buf, field_value.c_str(), sizeof(buf));
          buf[sizeof(buf) - 1] = '\0';
          if (inspector::property_text(field_name, buf, sizeof(buf))) {
            field_value = std::string(buf);
            changed = true;
          }
        } else {
          /// unknown type — display type name as read-only
          inspector::property_display(field_name, "<unsupported type>", colors::kTextDisabled);
        }

        return changed;
      }

      template <typename T>
      field_flags read_field_flags(T field) {
        field_flags f;
        if constexpr (refl::descriptor::has_attribute<attr::serializable>(field)) {
          constexpr auto& s = refl::descriptor::get_attribute<attr::serializable>(field);
          f.display_name = s.display_name;  // std::string_view; empty ⇒ use field name
          f.read_only = !s.editable;        // serializable(false) ⇒ read-only (transform.local_basis)
        }
        if constexpr (refl::descriptor::has_attribute<attr::asset_identifier_field>(field)) {
          f.asset_type = refl::descriptor::get_attribute<attr::asset_identifier_field>(field).asset_type;
        }
        if constexpr (refl::descriptor::has_attribute<attr::clamp>(field)) {
          const auto& c = refl::descriptor::get_attribute<attr::clamp>(field);
          f.has_range = true;
          f.range = glm::vec2{
            static_cast<float>(c.min),
            static_cast<float>(c.max)
          };
        }
        return f;
      }

      template <typename FT>
      bool draw_field(std::string_view label, FT& value, const field_context& ctx) {
        const type_key key = type_key_of<FT>();

        auto& field_editors = ctx.driver_ptr->get_field_editors();
        if (field_editors.has(key)) {
          return field_editors.edit(key, label, &value, ctx);
        }

        if constexpr (reflected_type<FT>) {
          bool changed = false;
          for_each(refl::reflect(value).members, [&](auto sub) {
            field_context child = ctx;
            child.flags = read_field_flags(sub);
            changed |= draw_field(child.flags.display_name.empty() ? std::string{ sub.name } : child.flags.display_name,
                                  sub(value), child);
          });
          return changed;
        }

        if constexpr (is_container<FT>) {
          bool changed = false;
          for (auto& item : value) {
            field_context child = ctx;
            changed |= draw_field("", item, child);
          }
          return changed;
        }

        bool modified = field_editors.edit(key, label, &value, ctx);  // draws "<unsupported type>"
        // clamp value if necessary

        if (ctx.flags.has_range) {
          if constexpr (is_linear_algebra_type<FT> &&
                        !(std::is_same_v<FT, glm::mat4> || std::is_same_v<FT, glm::mat3> || std::is_same_v<FT, glm::mat2>)) {
            for (int i = 0; i < value.length(); ++i) {
              if (value[i] < (float)ctx.flags.range.x) {
                value[i] = (float)ctx.flags.range.x;
                modified = true;
              }
              if (value[i] > (float)ctx.flags.range.y) {
                value[i] = (float)ctx.flags.range.y;
                modified = true;
              }
            }
          } else if constexpr (std::is_arithmetic_v<FT>) {
            if (value < static_cast<FT>(ctx.flags.range.x)) {
              value = static_cast<FT>(ctx.flags.range.x);
              modified = true;
            }
            if (value > static_cast<FT>(ctx.flags.range.y)) {
              value = static_cast<FT>(ctx.flags.range.y);
              modified = true;
            }
          } else if constexpr (std::is_floating_point_v<FT>) {
            if (value < static_cast<FT>(ctx.flags.range.x)) {
              value = static_cast<FT>(ctx.flags.range.x);
              modified = true;
            }
            if (value > static_cast<FT>(ctx.flags.range.y)) {
              value = static_cast<FT>(ctx.flags.range.y);
              modified = true;
            }
          }
        }

        return modified;
      }

    }  // namespace detail

    template <typename T>
    struct component_widget {
      bool operator()(const std::string_view name, T& component, scene* active_scene, scene_object* object, asset_handler* handler = nullptr, driver* drvr = nullptr) {
        OTHER_ASSERT(drvr != nullptr, "component_widget needs a driver for '{}'", name);
        bool changed = false;
        for_each(refl::reflect(component).members, [&](auto field) {
          field_context ctx{ active_scene, object, handler, drvr, detail::read_field_flags(field) };
          std::string label = ctx.flags.display_name.empty() ? std::string{ field.name } : ctx.flags.display_name;
          changed |= detail::draw_field(label, field(component), ctx);
        });
        return changed;
      }
    };

  }  // namespace ui
}  // namespace other

#endif  // OTHERLIB_UI_COMPONENT_WIDGET_HPP
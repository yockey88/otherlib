/**
 * \file ui/component_widget.hpp
 **/
#ifndef OTHERLIB_UI_COMPONENT_WIDGET_HPP
#define OTHERLIB_UI_COMPONENT_WIDGET_HPP

#include <string>
#include <string_view>

#include "math/orthonormal_basis.hpp"
#include "serialization/reflection.hpp"

#include "renderer/ui/colors.hpp"

#include "object/scene_object.hpp"
#include "scene/scene.hpp"

#include "driver/driver.hpp"
#include "ui/asset_browser_widgets.hpp"
#include "ui/inspector_widgets.hpp"

#include "asset/asset.hpp"
#include "asset/asset_handler.hpp"

namespace other {

  template <typename T>
  struct property_ui;

  namespace ui {

    // template <typename T>
    // struct component_widget {
    //   bool operator()(const std::string_view name, T& component, scene* active_scene, scene_object* object) {
    //     bool changed = false;
    //     for_each(refl::reflect(component).members, [&](const auto& field) {
    //       std::string field_name = std::string{ field.name };
    //       changed |= draw_field(field_name, field(component), active_scene, object);
    //     });
    //     return changed;
    //   }

    //   template <typename FT>
    //   bool draw_field(const std::string_view field_name, FT& field_value, scene* active_scene, scene_object* object) {
    //     {
    //       scoped_color color_text(ImGuiCol_Text, colors::rgba_to_imvec4(colors::kTextValueType));
    //       ImGui::Text("%s:", field_name.data());
    //     }

    //     /// \todo fix this
    //     bool changed = false;
    //     if constexpr (requires {
    //                     property_ui<FT>{}(std::declval<const std::string&>(), std::declval<FT&>(), active_scene, object);
    //                   }) {
    //       std::string unique_field_name = std::string(field_name);
    //       changed = property_ui<FT>{}(unique_field_name, field_value, active_scene, object);
    //     } else if constexpr (reflected_type<FT>) {
    //       for_each(refl::reflect(field_value).members, [&](const auto& sub_field) {
    //         std::string sub_field_name = std::string{ sub_field.name };
    //         changed |= draw_field(sub_field_name, sub_field(field_value), active_scene, object);
    //       });
    //     }
    //     /// check if overriden the special draw template
    //     else {
    //       shift_cursor_x(10.f);

    //       if constexpr (std::is_same_v<FT, bool>) {
    //         changed = ImGui::Checkbox(("##" + std::string{ field_name } + "_bool").c_str(), &field_value);
    //       } else if constexpr (std::is_integral_v<FT>) {
    //         if constexpr (sizeof(FT) <= sizeof(int)) {
    //           changed = ImGui::DragInt(("##" + std::string{ field_name } + "_int").c_str(), reinterpret_cast<int*>(&field_value));
    //         } else {
    //           changed = ImGui::DragScalar(("##" + std::string{ field_name } + "_int64").c_str(), ImGuiDataType_S64, &field_value);
    //         }
    //       } else if constexpr (std::is_floating_point_v<FT>) {
    //         if constexpr (std::same_as<FT, float>) {
    //           changed = ImGui::DragFloat(("##" + std::string{ field_name } + "_float").c_str(), &field_value);
    //         } else {
    //           changed = ImGui::DragScalar(("##" + std::string{ field_name } + "_double").c_str(), ImGuiDataType_Double, &field_value);
    //         }
    //       }
    //       // Fallback for unsupported types
    //       else {
    //         ImGui::Text("Unsupported type for field '%s'", field_name.data());
    //       }
    //       shift_cursor_x(-10.f);
    //     }

    //     return changed;
    //   }
    // };

    namespace detail {

      template <typename FT>
      concept has_property_ui = requires {
        property_ui<FT>{}(std::declval<const std::string&>(), std::declval<FT&>(), std::declval<scene*>(), std::declval<scene_object*>());
      };

      inline bool is_asset_id_field(const std::string& field_name) {
        return field_name.find("asset_id") != std::string::npos;
      }

      inline std::vector<asset::type> accepted_asset_types_for_field(const std::string& field_name) {
        if (field_name.find("model") != std::string::npos) {
          return { asset::MODEL_SOURCE, asset::MODEL };
        }
        if (field_name.find("script") != std::string::npos) {
          return { asset::SCRIPT_SOURCE, asset::SCRIPT };
        }
        if (field_name.find("texture") != std::string::npos) {
          return { asset::TEXTURE };
        }
        if (field_name.find("audio") != std::string::npos || field_name.find("sound") != std::string::npos) {
          return { asset::AUDIO };
        }
        if (field_name.find("animation") != std::string::npos || field_name.find("anim") != std::string::npos) {
          return { asset::ANIMATION };
        }
        if (field_name.find("scene") != std::string::npos) {
          return { asset::SCENE };
        }
        return {};
      }

      inline std::string asset_display_name_for_id(natural_t asset_id, const asset_handler* handler) {
        if (asset_id == 0 || handler == nullptr) {
          return "None";
        }
        const asset* a = handler->get_loaded_asset(asset_id);
        if (a != nullptr) {
          return a->path.filename().string();
        }
        asset_state state = handler->get_asset_state(asset_id);
        if (state == asset_state::LOADING) {
          return "Loading...";
        }
        return std::format("Asset #{}", asset_id);
      }

      template <typename FT>
      bool draw_inspector_field(const std::string& field_name, FT& field_value, scene* active_scene, scene_object* object, asset_handler* handler = nullptr, driver* drvr = nullptr, std::string display_name = "") {
        bool changed = false;

        if (display_name.empty()) {
          display_name = field_name;
        }

        if constexpr (has_property_ui<FT>) {
          changed = property_ui<FT>{}(field_name, field_value, active_scene, object);
        } else if constexpr (std::is_same_v<FT, natural_t>) {
          if (is_asset_id_field(field_name)) {
            std::string asset_name = asset_display_name_for_id(field_value, handler);
            auto accepted = accepted_asset_types_for_field(field_name);
            std::string dropped_path;
            if (inspector::property_asset_slot(display_name, field_value, asset_name, accepted, dropped_path)) {
              if (drvr != nullptr) {
                natural_t new_id = drvr->begin_asset_load(dropped_path);
                if (new_id != 0) {
                  field_value = new_id;
                  changed = true;
                }
              }
            }
          } else {
            int32_t ival = static_cast<int32_t>(field_value);
            if (inspector::property_int(display_name, ival)) {
              field_value = static_cast<natural_t>(ival);
              changed = true;
            }
          }
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
          /// 3. recursive reflected type — iterate sub-fields
          for_each(refl::reflect(field_value).members, [&](const auto& sub_field) {
            std::string sub_name = std::string{ sub_field.name };
            changed |= draw_inspector_field(sub_name, sub_field(field_value), active_scene, object, handler, drvr);
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
          int32_t ival = static_cast<int32_t>(field_value);
          if (inspector::property_int(field_name, ival)) {
            field_value = static_cast<FT>(ival);
            changed = true;
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

    }  // namespace detail

    template <typename T>
    struct component_widget {
      bool operator()(const std::string_view name, T& component, scene* active_scene, scene_object* object, asset_handler* handler = nullptr, driver* drvr = nullptr) {
        bool changed = false;
        for_each(refl::reflect(component).members, [&](auto field) {
          std::string field_name = std::string{ field.name };
          std::string display_name = "";

          if constexpr (refl::descriptor::has_attribute<attr::serializable>(field)) {
            constexpr auto& serializable_attr = refl::descriptor::get_attribute<attr::serializable>(field);
            if (!serializable_attr.display_name.empty()) {
              display_name = std::string(serializable_attr.display_name);
            }
          }
          changed |= detail::draw_inspector_field(field_name, field(component), active_scene, object, handler, drvr, display_name);
        });
        return changed;
      }
    };

  }  // namespace ui
}  // namespace other

#endif  // OTHERLIB_UI_COMPONENT_WIDGET_HPP
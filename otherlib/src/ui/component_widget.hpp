/**
 * \file ui/component_widget.hpp
 **/
#ifndef OTHERLIB_UI_COMPONENT_WIDGET_HPP
#define OTHERLIB_UI_COMPONENT_WIDGET_HPP

#include <string>
#include <string_view>

#include "serialization/reflection.hpp"

#include "renderer/ui/ui_helpers.hpp"

#include "object/scene_object.hpp"
#include "scene/scene.hpp"

#include "ui/colors.hpp"

namespace other {

  template <typename T>
  struct property_ui;

  namespace ui {

    template <typename T>
    struct component_widget {
      bool operator()(const std::string_view name, T& component, scene* active_scene, scene_object* object) {
        bool changed = false;
        for_each(refl::reflect(component).members, [&](const auto& field) {
          std::string field_name = std::string{ field.name };
          changed |= draw_field(field_name, field(component), active_scene, object);
        });
        return changed;
      }

      template <typename FT>
      bool draw_field(const std::string_view field_name, FT& field_value, scene* active_scene, scene_object* object) {
        {
          scoped_color color_text(ImGuiCol_Text, colors::rgba_to_imvec4(colors::kTextValueType));
          ImGui::Text("%s:", field_name.data());
        }

        /// \todo fix this
        bool changed = false;
        if constexpr (requires {
                        property_ui<FT>{}(std::declval<const std::string&>(), std::declval<FT&>(), active_scene, object);
                      }) {
          std::string unique_field_name = std::string(field_name);
          changed = property_ui<FT>{}(unique_field_name, field_value, active_scene, object);
        } else if constexpr (reflected_type<FT>) {
          for_each(refl::reflect(field_value).members, [&](const auto& sub_field) {
            std::string sub_field_name = std::string{ sub_field.name };
            changed |= draw_field(sub_field_name, sub_field(field_value), active_scene, object);
          });
        }
        /// check if overriden the special draw template
        else {
          shift_cursor_x(10.f);

          if constexpr (std::is_same_v<FT, bool>) {
            changed = ImGui::Checkbox(("##" + std::string{ field_name } + "_bool").c_str(), &field_value);
          } else if constexpr (std::is_integral_v<FT>) {
            if constexpr (sizeof(FT) <= sizeof(int)) {
              changed = ImGui::DragInt(("##" + std::string{ field_name } + "_int").c_str(), reinterpret_cast<int*>(&field_value));
            } else {
              changed = ImGui::DragScalar(("##" + std::string{ field_name } + "_int64").c_str(), ImGuiDataType_S64, &field_value);
            }
          } else if constexpr (std::is_floating_point_v<FT>) {
            if constexpr (std::same_as<FT, float>) {
              changed = ImGui::DragFloat(("##" + std::string{ field_name } + "_float").c_str(), &field_value);
            } else {
              changed = ImGui::DragScalar(("##" + std::string{ field_name } + "_double").c_str(), ImGuiDataType_Double, &field_value);
            }
          }
          // Fallback for unsupported types
          else {
            ImGui::Text("Unsupported type for field '%s'", field_name.data());
          }
          shift_cursor_x(-10.f);
        }

        return changed;
      }
    };

  }  // namespace ui
}  // namespace other

#endif  // OTHERLIB_UI_COMPONENT_WIDGET_HPP
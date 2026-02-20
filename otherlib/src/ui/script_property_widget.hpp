/**
 * \file ui/script_property_widget.hpp
 **/
#ifndef OTHERLIB_UI_SCRIPT_PROPERTY_WIDGET_HPP
#define OTHERLIB_UI_SCRIPT_PROPERTY_WIDGET_HPP

#include <string>

#include "core/defines.hpp"
#include "core/subsystem.hpp"

#include "dotnet/behavior_descriptor.hpp"
#include "renderer/ui/colors.hpp"
#include "script/scripting_environment.hpp"

#include "object/script_component.hpp"
#include "scene/scene.hpp"

#include "ui/component_widget.hpp"
#include "ui/inspector_widgets.hpp"

#include "imgui.h"

namespace other {
  namespace ui {

    bool draw_behavior_field(integer_t script_object_id, const behavior_descriptor& behavior, const behavior_field_descriptor& field);
    bool begin_behavior_section(const std::string_view behavior_name, const std::string_view full_type_name, int32_t behavior_index);
    void end_behavior_section();

    void draw_no_behaviors_message();
    void draw_field_tooltip(const std::string_view tooltip);
    bool begin_field_group(const std::string_view group_name);
    void end_field_group(bool was_open);

    // template <>
    // struct component_widget<script_component> {
    //   bool operator()(const std::string_view name, script_component& component, scene* active_scene, scene_object* object, asset_handler* handler = nullptr, driver* drvr = nullptr) {
    //     bool changed = false;

    //     auto* env = subsystem<scripting_environment>::get();
    //     if (env == nullptr) {
    //       inspector::property_display("Status", "Scripting environment unavailable", colors::kTextDisabled);
    //       return false;
    //     }

    //     auto* script = active_scene->get_component<script_component>(object);
    //     OTHER_ASSERT(script != nullptr, "Script component not found on object with ID {}", object->id);

    //     inspector::property_display("Script ID", std::format("{}", component.script_object_id), colors::kTextMuted);

    //     script_object* script_obj = env->get_object(component.script_object_id);
    //     if (script_obj == nullptr) {
    //       inspector::property_display("Status", "No Script Object Attached", colors::kTextDisabled);
    //       return false;
    //     }

    //     behavior_snapshot snapshot = script_obj->get_behavior_snapshot();
    //     if (!snapshot.valid) {
    //       inspector::property_display("Status", "No .NET object", colors::kTextDisabled);
    //       return false;
    //     }

    //     if (snapshot.behaviors.empty()) {
    //       draw_no_behaviors_message();
    //       return false;
    //     }

    //     /// render each behavior as a sub-section
    //     for (const auto& behavior : snapshot.behaviors) {
    //       bool section_open = begin_behavior_section(behavior.display_name, behavior.full_type_name, behavior.behavior_index);

    //       if (section_open) {
    //         std::string current_group;
    //         bool group_open = false;

    //         for (const auto& field : behavior.fields) {
    //           /// handle group transitions
    //           if (has_flag(field.flags, behavior_display_flags::is_group_start)) {
    //             if (group_open) {
    //               end_field_group(group_open);
    //             }
    //             current_group = field.group_name;
    //             group_open = begin_field_group(current_group);
    //             if (!group_open) {
    //               continue;
    //             }
    //           } else if (!current_group.empty() && !has_flag(field.flags, behavior_display_flags::is_group_start)) {
    //             /// still inside a group — skip if group is collapsed
    //             if (!group_open) {
    //               continue;
    //             }
    //           }

    //           /// handle separator
    //           if (has_flag(field.flags, behavior_display_flags::has_separator)) {
    //             ImGui::Separator();
    //           }

    //           /// draw the field
    //           changed |= draw_behavior_field(component.script_object_id, behavior, field);
    //         }

    //         /// close any open group
    //         if (group_open) {
    //           end_field_group(group_open);
    //         }
    //       }

    //       end_behavior_section();
    //     }

    //     return changed;
    //   }
    // };

  }  // namespace ui
}  // namespace other

#endif  // OTHERLIB_UI_SCRIPT_PROPERTY_WIDGET_HPP
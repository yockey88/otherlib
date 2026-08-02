/**
 * \file ui/scene-hierarchy/scene_hierarchy.cpp
 **/
#include "ui/scene-hierarchy/scene_hierarchy.hpp"

#include "driver/driver.hpp"
#include "driver/systems/scene_system.hpp"
#include "theme/colors.hpp"
#include "ui/scene-hierarchy/hierarchy_widgets.hpp"
#include "ui/ui_node.hpp"

#include "imgui.h"

namespace other {
  namespace ui {

    scene_hierarchy::scene_hierarchy(editor_context& ctx, event_system& events, driver* drvr)
        : ui_window(&events, "Scene Hierarchy"), editor_ctx(ctx), driver_ptr(drvr) {
    }

    void scene_hierarchy::on_render_body() {
      auto& scenes = driver_ptr->get_kernel().get_core_system<scene_system>();
      auto* active_scene = scenes.get_active_scene();

      if (active_scene == nullptr) {
        hierarchy::draw_empty_scene_message();
        return;
      }

      const bool mouse_right_release = ImGui::IsMouseReleased(ImGuiMouseButton_Right);
      const bool is_hierarchy_panel_hovered = ImGui::IsWindowHovered();
      if (is_hierarchy_panel_hovered && mouse_right_release) {
        ImGui::OpenPopup("##hierarchy_ctx_menu");
      }
      draw_hierarchy_context_menu(active_scene);

      ImGui::PushStyleColor(ImGuiCol_ChildBg, colors::rgba_to_imvec4(colors::hierarchy::kBG));
      if (!ImGui::BeginChild("##scene-hierarchy", { 0, 0 }, 0, ImGuiWindowFlags_HorizontalScrollbar)) {
        ImGui::PopStyleColor();
        ImGui::EndChild();
        return;
      }

      if (hierarchy::draw_search_bar(filter_buf, sizeof(filter_buf))) {
        filter_lower = filter_buf;
        std::transform(filter_lower.begin(), filter_lower.end(), filter_lower.begin(), [](unsigned char c) { return std::tolower(c); });
      }
      ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));

      auto& root = active_scene->root_object();
      const auto root_children = active_scene->get_children_ids(root.id);
      {
        bool has_root_objects = !root_children.empty();

        hierarchy::item_flags root_flags{};
        root_flags.selected = false;
        root_flags.has_children = has_root_objects;
        root_flags.expanded = true;
        root_flags.visible = true;
        root_flags.indent_level = 0;

        auto _ = hierarchy::draw_item(active_scene->name, 0, root_flags);
      }
      for (natural_t root_id : root_children) {
        scene_object* obj = active_scene->find_object(root_id);
        OTHER_ASSERT(obj != nullptr, "Scene hierarchy: root child id {} not found in scene", root_id);
        if (!passes_filter(active_scene, *obj)) {
          continue;
        }
        draw_object_tree(active_scene, *obj, 1);
      }

      ImGui::PopStyleVar();

      if (renaming_object_id != 0) {
        scene_object* obj = active_scene->find_object(renaming_object_id);
        if (obj != nullptr) {
          ImGui::SetNextWindowPos(ImGui::GetMousePos(), ImGuiCond_Appearing);
          if (ImGui::BeginPopup("##hier_rename")) {
            ImGui::PushStyleColor(ImGuiCol_FrameBg, colors::rgba_to_imvec4(colors::hierarchy::kSearchBG));
            ImGui::PushStyleColor(ImGuiCol_Border, colors::rgba_to_imvec4(colors::hierarchy::kSearchBorderFocused));

            if (ImGui::InputText("##rename_input", rename_buf, sizeof(rename_buf), ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll)) {
              obj->name = std::string(rename_buf);
              editor_ctx.notify_scene_edited();
              renaming_object_id = 0;
              ImGui::CloseCurrentPopup();
            }

            /// close on escape or clicking away
            if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
              renaming_object_id = 0;
              ImGui::CloseCurrentPopup();
            }

            ImGui::PopStyleColor(2);
            ImGui::EndPopup();
          }
        } else {
          renaming_object_id = 0;
        }
      }

      ImGui::PopStyleColor();
      ImGui::EndChild();

      if (editor_ctx.has_selection()) {
        draw_object_context_menu(active_scene, active_scene->get_object(editor_ctx.current_selection.objects.front()));
      } else {
      }
    }

    void scene_hierarchy::draw_object_context_menu(scene* active_scene, scene_object& object) {
      if (!ImGui::BeginPopup("##object_settings_ctx_menu")) {
        return;
      }

      if (ImGui::MenuItem("Delete")) {
        /// the destroyed id must leave the selection before anything dereferences it
        editor_ctx.current_selection.objects.clear();
        active_scene->destroy_object(object.id);
        editor_ctx.notify_scene_edited();
        ImGui::CloseCurrentPopup();
      }

      ImGui::EndPopup();
    }

    void scene_hierarchy::draw_hierarchy_context_menu(scene* active_scene) {
      if (!ImGui::BeginPopup("##hierarchy_ctx_menu")) {
        return;
      }

      if (ImGui::MenuItem("Create Empty Object")) {
        if (active_scene != nullptr) {
          auto& new_obj = active_scene->create_object("New Object");
          editor_ctx.select_object(new_obj.id);
          editor_ctx.notify_scene_edited();
        } else {
          CORE_LOG_ERROR("Cannot create object: no active scene.");
        }
      }

      ImGui::EndPopup();
    }

    bool scene_hierarchy::passes_filter(scene* active_scene, scene_object& object) const {
      if (filter_lower.empty()) {
        return true;
      }

      /// check this object's name (case-insensitive)
      std::string name_lower = object.name;
      std::transform(name_lower.begin(), name_lower.end(), name_lower.begin(), [](unsigned char c) { return std::tolower(c); });
      if (name_lower.find(filter_lower) != std::string::npos) {
        return true;
      }

      /// check descendants recursively
      for (natural_t child_id : active_scene->get_children_ids(object.id)) {
        scene_object* child = active_scene->find_object(child_id);
        if (child != nullptr && passes_filter(active_scene, *child)) {
          return true;
        }
      }

      return false;
    }

    void scene_hierarchy::draw_object_tree(scene* active_scene, scene_object& object, uint32_t indent_level) {
      const auto child_ids = active_scene->get_children_ids(object.id);
      bool has_children = !child_ids.empty();

      float row_y = ImGui::GetCursorScreenPos().y;
      hierarchy::draw_indent_guide(indent_level, row_y, hierarchy::kItemHeight);

      hierarchy::item_flags flags{};
      flags.selected = editor_ctx.has_selection() && std::ranges::find(editor_ctx.current_selection.objects, object.id) != editor_ctx.current_selection.objects.end();
      flags.has_children = has_children;
      flags.visible = object.visible;
      flags.disabled = !object.visible;
      flags.indent_level = indent_level;

      bool is_expanded = flags.selected;
      flags.expanded = is_expanded;
      auto interaction = hierarchy::draw_item(object.name, object.id, flags);

#define DEBUG_DRAW 0
#if DEBUG_DRAW
      /// draw flags
      ImGui::Text(
        "Flags:\nflags.selected = %s\nflags.has_children = %s\nflags.expanded = %s\nflags.visible = %s\nflags.disabled = %s",
        flags.selected ? "true" : "false",
        flags.has_children ? "true" : "false",
        flags.expanded ? "true" : "false",
        flags.visible ? "true" : "false",
        flags.disabled ? "true" : "false");
      /// draw interaction
      ImGui::Text(
        "Interaction:\nclicked = %s\ndouble_clicked = %s\nright_clicked = %s\nexpand_toggled = %s\nvisibility_toggled = %s",
        interaction.clicked ? "true" : "false",
        interaction.double_clicked ? "true" : "false",
        interaction.right_clicked ? "true" : "false",
        interaction.expand_toggled ? "true" : "false",
        interaction.visibility_toggled ? "true" : "false");
#endif

      if (interaction.clicked) {
        editor_ctx.select_object(object.id);
      }

      if (interaction.expand_toggled) {
        if (is_expanded) {
          editor_ctx.deselect_object(object.id);
        } else {
          editor_ctx.select_object(object.id);
        }
      }

      if (interaction.visibility_toggled) {
        object.visible = !object.visible;
        editor_ctx.notify_scene_edited();
      }

      if (interaction.double_clicked) {
        /// initiate rename
        renaming_object_id = object.id;
        std::strncpy(rename_buf, object.name.c_str(), sizeof(rename_buf));
        rename_buf[sizeof(rename_buf) - 1] = '\0';
        ImGui::OpenPopup("##hier_rename");
      }

      if (interaction.right_clicked) {
        editor_ctx.select_object(object.id);
        ImGui::OpenPopup("##object_settings_ctx_menu");
      }

      if (has_children && is_expanded) {
        for (natural_t child_id : child_ids) {
          scene_object* child = active_scene->find_object(child_id);
          OTHER_ASSERT(child != nullptr, "Scene hierarchy: child id {} not found in scene", child_id);
          if (!passes_filter(active_scene, *child)) {
            continue;
          }

          draw_object_tree(active_scene, *child, indent_level + 1);
        }
      }
    }

  }  // namespace ui
}  // namespace other
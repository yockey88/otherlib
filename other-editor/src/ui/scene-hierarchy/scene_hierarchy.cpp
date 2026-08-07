/**
 * \file ui/scene-hierarchy/scene_hierarchy.cpp
 **/
#include "ui/scene-hierarchy/scene_hierarchy.hpp"

#include "core/profiler.hpp"
#include "driver/driver.hpp"
#include "driver/systems/scene_system.hpp"
#include "theme/colors.hpp"
#include "ui/scene-hierarchy/hierarchy_widgets.hpp"
#include "ui/ui_node.hpp"

#include "imgui.h"

namespace other {
  namespace ui {

    namespace {
      constexpr const char* kHierarchyDragPayload = "OTHER_HIERARCHY_OBJECT_DRAG";
    }  // namespace

    scene_hierarchy::scene_hierarchy(editor_context& ctx, event_system& events, driver* drvr)
        : ui_window(&events, "Scene Hierarchy"), editor_ctx(ctx), driver_ptr(drvr) {
    }

    void scene_hierarchy::on_render_body() {
      PROFILE_SECTION("scene_hierarchy::on_render_body");
      auto& scenes = driver_ptr->get_kernel().get_core_system<scene_system>();
      auto* active_scene = scenes.get_active_scene();

      if (active_scene == nullptr) {
        hierarchy::draw_empty_scene_message();
        return;
      }

      ImGui::PushStyleColor(ImGuiCol_ChildBg, colors::rgba_to_imvec4(colors::hierarchy::kBG));
      if (!ImGui::BeginChild("##scene-hierarchy", { 0, 0 }, 0, ImGuiWindowFlags_HorizontalScrollbar)) {
        ImGui::EndChild();
        ImGui::PopStyleColor();
        return;
      }

      if (hierarchy::draw_search_bar(filter_buf, sizeof(filter_buf))) {
        filter_lower = filter_buf;
        std::transform(filter_lower.begin(), filter_lower.end(), filter_lower.begin(), [](unsigned char c) { return std::tolower(c); });
      }

      bool row_right_clicked = false;

      ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));

      auto& root = active_scene->root_object();
      const auto root_children = active_scene->get_children_ids(root.id);
      {
        hierarchy::item_flags root_flags{};
        root_flags.selected = false;
        root_flags.has_children = !root_children.empty();
        root_flags.expanded = true;
        root_flags.visible = true;
        root_flags.indent_level = 0;

        auto _ = hierarchy::draw_item(active_scene->name, 0, root_flags);
        /// dropping on the scene row moves an object back to the top level
        accept_reparent_drop(root.id);
      }
      for (natural_t root_id : root_children) {
        scene_object* obj = active_scene->find_object(root_id);
        OTHER_ASSERT(obj != nullptr, "Scene hierarchy: root child id {} not found in scene", root_id);
        if (!passes_filter(active_scene, *obj)) {
          continue;
        }
        draw_object_tree(active_scene, *obj, 1, row_right_clicked);
      }

      ImGui::PopStyleVar();

      if (pending_reparent.has_value()) {
        if (active_scene->reparent_object(pending_reparent->first, pending_reparent->second)) {
          expanded_ids.insert(pending_reparent->second);
          editor_ctx.notify_scene_edited();
        }
        pending_reparent.reset();
      }

      /// right-press on empty space; row right-presses open the object menu instead
      if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right) && !row_right_clicked) {
        ImGui::OpenPopup("##hierarchy_ctx_menu");
      }

      /// Del / F2 while the panel has focus and no text field is capturing input
      if (ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows) && editor_ctx.has_selection() &&
          renaming_object_id == 0 && !ImGui::GetIO().WantTextInput) {
        if (ImGui::IsKeyPressed(ImGuiKey_Delete)) {
          auto doomed = editor_ctx.current_selection.objects;
          editor_ctx.current_selection.objects.clear();
          for (natural_t id : doomed) {
            if (active_scene->find_object(id) != nullptr) {
              active_scene->destroy_object(id);
            }
          }
          editor_ctx.notify_scene_edited();
        } else if (ImGui::IsKeyPressed(ImGuiKey_F2)) {
          scene_object* obj = active_scene->find_object(editor_ctx.current_selection.objects.front());
          if (obj != nullptr) {
            begin_rename(*obj);
          }
        }
      }

      draw_hierarchy_context_menu(active_scene);
      if (editor_ctx.has_selection()) {
        /// the selected id can be stale right after a snapshot restore — no menu for
        ///  ids that no longer resolve
        scene_object* selected = active_scene->find_object(editor_ctx.current_selection.objects.front());
        if (selected != nullptr) {
          draw_object_context_menu(active_scene, *selected);
        }
      }
      draw_rename_popup(active_scene);

      ImGui::EndChild();
      ImGui::PopStyleColor();
    }

    void scene_hierarchy::begin_rename(scene_object& object) {
      renaming_object_id = object.id;
      std::strncpy(rename_buf, object.name.c_str(), sizeof(rename_buf));
      rename_buf[sizeof(rename_buf) - 1] = '\0';
      /// the popup opens outside whatever popup/menu requested the rename, else it would
      ///  nest under it and close with it
      rename_popup_pending = true;
    }

    void scene_hierarchy::draw_rename_popup(scene* active_scene) {
      if (rename_popup_pending) {
        ImGui::OpenPopup("##hier_rename");
        rename_popup_pending = false;
      }

      if (renaming_object_id == 0) {
        return;
      }

      scene_object* obj = active_scene->find_object(renaming_object_id);
      if (obj == nullptr) {
        renaming_object_id = 0;
        return;
      }

      ImGui::SetNextWindowPos(ImGui::GetMousePos(), ImGuiCond_Appearing);
      if (ImGui::BeginPopup("##hier_rename")) {
        ImGui::PushStyleColor(ImGuiCol_FrameBg, colors::rgba_to_imvec4(colors::hierarchy::kSearchBG));
        ImGui::PushStyleColor(ImGuiCol_Border, colors::rgba_to_imvec4(colors::hierarchy::kSearchBorderFocused));

        if (ImGui::IsWindowAppearing()) {
          ImGui::SetKeyboardFocusHere();
        }
        if (ImGui::InputText("##rename_input", rename_buf, sizeof(rename_buf), ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll)) {
          obj->name = std::string(rename_buf);
          editor_ctx.notify_scene_edited();
          renaming_object_id = 0;
          ImGui::CloseCurrentPopup();
        }

        if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
          renaming_object_id = 0;
          ImGui::CloseCurrentPopup();
        }

        ImGui::PopStyleColor(2);
        ImGui::EndPopup();
      } else {
        /// dismissed by clicking away
        renaming_object_id = 0;
      }
    }

    void scene_hierarchy::accept_reparent_drop(natural_t new_parent_id) {
      if (!ImGui::BeginDragDropTarget()) {
        return;
      }
      if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(kHierarchyDragPayload); payload != nullptr) {
        const natural_t dragged = *static_cast<const natural_t*>(payload->Data);
        if (dragged != new_parent_id) {
          pending_reparent = std::make_pair(dragged, new_parent_id);
        }
      }
      ImGui::EndDragDropTarget();
    }

    void scene_hierarchy::draw_object_context_menu(scene* active_scene, scene_object& object) {
      if (!ImGui::BeginPopup("##object_settings_ctx_menu")) {
        return;
      }

      ImGui::TextDisabled("%s", object.name.c_str());
      ImGui::Separator();

      if (ImGui::MenuItem("Create Child")) {
        auto& new_obj = active_scene->create_object("New Object", &object);
        expanded_ids.insert(object.id);
        editor_ctx.current_selection.objects.clear();
        editor_ctx.select_object(new_obj.id);
        editor_ctx.notify_scene_edited();
      }

      if (ImGui::MenuItem("Rename", "F2")) {
        begin_rename(object);
      }

      if (ImGui::MenuItem("Delete", "Del")) {
        /// the destroyed id must leave the selection before anything dereferences it
        editor_ctx.current_selection.objects.clear();
        active_scene->destroy_object(object.id);
        editor_ctx.notify_scene_edited();
      }

      ImGui::EndPopup();
    }

    void scene_hierarchy::draw_hierarchy_context_menu(scene* active_scene) {
      if (!ImGui::BeginPopup("##hierarchy_ctx_menu")) {
        return;
      }

      if (ImGui::MenuItem("Create Empty Object")) {
        auto& new_obj = active_scene->create_object("New Object");
        editor_ctx.current_selection.objects.clear();
        editor_ctx.select_object(new_obj.id);
        editor_ctx.notify_scene_edited();
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

    void scene_hierarchy::draw_object_tree(scene* active_scene, scene_object& object, uint32_t indent_level, bool& row_right_clicked) {
      const auto child_ids = active_scene->get_children_ids(object.id);
      const bool has_children = !child_ids.empty();

      float row_y = ImGui::GetCursorScreenPos().y;
      hierarchy::draw_indent_guide(indent_level, row_y, hierarchy::kItemHeight);

      /// an active search force-expands so matches inside collapsed branches show
      const bool is_expanded = expanded_ids.contains(object.id) || !filter_lower.empty();

      hierarchy::item_flags flags{};
      flags.selected = editor_ctx.has_selection() && std::ranges::find(editor_ctx.current_selection.objects, object.id) != editor_ctx.current_selection.objects.end();
      flags.has_children = has_children;
      flags.visible = object.visible;
      flags.disabled = !object.visible;
      flags.indent_level = indent_level;
      flags.expanded = is_expanded;

      auto interaction = hierarchy::draw_item(object.name, object.id, flags);

      if (ImGui::BeginDragDropSource()) {
        const natural_t payload_id = object.id;
        ImGui::SetDragDropPayload(kHierarchyDragPayload, &payload_id, sizeof(payload_id));
        ImGui::TextUnformatted(object.name.c_str());
        ImGui::EndDragDropSource();
      }
      accept_reparent_drop(object.id);

      if (interaction.clicked) {
        if (ImGui::GetIO().KeyCtrl) {
          /// ctrl-click toggles membership in a multi-selection
          auto& sel = editor_ctx.current_selection.objects;
          if (auto it = std::ranges::find(sel, object.id); it != sel.end()) {
            sel.erase(it);
          } else {
            sel.push_back(object.id);
          }
        } else {
          editor_ctx.current_selection.objects.clear();
          editor_ctx.select_object(object.id);
        }
      }

      if (interaction.expand_toggled) {
        if (is_expanded) {
          expanded_ids.erase(object.id);
        } else {
          expanded_ids.insert(object.id);
        }
      }

      if (interaction.visibility_toggled) {
        object.visible = !object.visible;
        editor_ctx.notify_scene_edited();
      }

      if (interaction.double_clicked) {
        begin_rename(object);
      }

      if (interaction.right_clicked) {
        row_right_clicked = true;
        if (!flags.selected) {
          editor_ctx.current_selection.objects.clear();
          editor_ctx.select_object(object.id);
        }
        ImGui::OpenPopup("##object_settings_ctx_menu");
      }

      if (has_children && is_expanded) {
        for (natural_t child_id : child_ids) {
          scene_object* child = active_scene->find_object(child_id);
          OTHER_ASSERT(child != nullptr, "Scene hierarchy: child id {} not found in scene", child_id);
          if (!passes_filter(active_scene, *child)) {
            continue;
          }

          draw_object_tree(active_scene, *child, indent_level + 1, row_right_clicked);
        }
      }
    }

  }  // namespace ui
}  // namespace other

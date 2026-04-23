/**
 * \file ui/scene-hierarchy/scene_hierarchy_node.cpp
 **/
#include "ui/scene-hierarchy/scene_hierarchy_node.hpp"

#include "renderer/ui/colors.hpp"

#include "driver/driver.hpp"
#include "driver/systems/scene_system.hpp"

namespace other {
  namespace ui {

    scene_hierarchy_node::scene_hierarchy_node(ui_window* window, driver* drvr)
        : ui_node(window, "Scene Hierarchy"), driver_ptr(drvr) {
      /// listen for external selection requests (e.g. from viewport click)
      // events().add_listener("ui.viewport.object-selected", [this](const value& data) {
      //   if (data.type() != value_type::UINT64) {
      //     return;
      //   }
      //   natural_t object_id = data;
      //   selected_object_id = object_id;

      //   /// \todo
      //   // /// auto-expand parents so the selected item is visible
      //   // auto* active_scene = driver_ptr->get_active_scene();
      //   // if (active_scene != nullptr) {
      //   //   /// walk up the parent chain and expand each ancestor
      //   //   scene_object* obj = active_scene->find_object(object_id);
      //   //   while (obj != nullptr && obj->id != 0) {
      //   //     scene_object* parent = active_scene->get_parent(obj->id);
      //   //     if (parent != nullptr) {
      //   //       expanded_ids.insert(parent->id);
      //   //       obj = parent;
      //   //     } else {
      //   //       obj = nullptr;
      //   //     }
      //   //   }
      //   // }
      // });
    }

    bool scene_hierarchy_node::passes_filter(scene* active_scene, scene_object& object) const {
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

    void scene_hierarchy_node::on_render_node_body() {
      ImGui::PushStyleColor(ImGuiCol_ChildBg, colors::rgba_to_imvec4(colors::hierarchy::kBG));

      /// get num objects and calculate need height
      ImVec2 size = ImVec2(0, ImGui::GetContentRegionAvail().y / 4.f);

      auto& scenes = driver_ptr->get_kernel().get_core_system<scene_system>();
      auto* active_scene = scenes.get_active_scene();
      if (active_scene == nullptr) {
        if (!ImGui::BeginChild("##scene-hierarchy", size)) {
          ImGui::EndChild();
          return;
        }

        hierarchy::draw_empty_scene_message();
        ImGui::EndChild();
        ImGui::PopStyleColor();
        return;
      }

      if (selected_object_id == 0) {
        size = ImVec2(0, 0);
      }

      if (!ImGui::BeginChild("##scene-hierarchy", size, false, ImGuiWindowFlags_HorizontalScrollbar)) {
        ImGui::PopStyleColor();
        ImGui::EndChild();
        return;
      }

      if (hierarchy::draw_search_bar(filter_buf, sizeof(filter_buf))) {
        filter_lower = filter_buf;
        std::transform(filter_lower.begin(), filter_lower.end(), filter_lower.begin(), [](unsigned char c) { return std::tolower(c); });

        /// when filtering, auto - expand everything so matches are visible
        if (!filter_lower.empty()) {
          for (const auto& id : active_scene->get_all_object_ids()) {
            scene_object* obj = active_scene->find_object(id);
            auto children_ids = active_scene->get_children_ids(obj->id);
            if (obj != nullptr && !children_ids.empty()) {
              expanded_ids.insert(id);
            }
          }
        }
      }
      ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));

      const std::string& scene_name = active_scene->name;
      auto& root = active_scene->root_object();
      const auto root_children = active_scene->get_children_ids(root.id);
      {
        bool has_root_objects = !root_children.empty();
        bool scene_expanded = expanded_ids.contains(0);  // id 0 = scene root pseudo-node

        hierarchy::item_flags root_flags{};
        root_flags.selected = (selected_object_id == 0);
        root_flags.has_children = has_root_objects;
        root_flags.expanded = scene_expanded;
        root_flags.visible = true;
        root_flags.is_scene_root = true;
        root_flags.indent_level = 0;

        auto interaction = hierarchy::draw_item(scene_name, 0, root_flags);

        if (interaction.clicked) {
          select_object(0);
        }
        if (interaction.expand_toggled) {
          if (scene_expanded) {
            expanded_ids.erase(0);
          } else {
            expanded_ids.insert(0);
          }
        }
      }

      bool scene_expanded = expanded_ids.contains(0);

      if (scene_expanded) {
        for (natural_t root_id : root_children) {
          scene_object* obj = active_scene->find_object(root_id);
          if (obj == nullptr) continue;

          if (!passes_filter(active_scene, *obj)) continue;

          draw_object_tree(active_scene, *obj, 1);
        }
      }

      ImGui::PopStyleVar();

      draw_context_menu(active_scene);

      if (renaming_object_id != 0) {
        scene_object* obj = active_scene->find_object(renaming_object_id);
        if (obj != nullptr) {
          ImGui::SetNextWindowPos(ImGui::GetMousePos(), ImGuiCond_Appearing);
          if (ImGui::BeginPopup("##hier_rename")) {
            ImGui::PushStyleColor(ImGuiCol_FrameBg, colors::rgba_to_imvec4(colors::hierarchy::kSearchBG));
            ImGui::PushStyleColor(ImGuiCol_Border, colors::rgba_to_imvec4(colors::hierarchy::kSearchBorderFocused));

            if (ImGui::InputText("##rename_input", rename_buf, sizeof(rename_buf), ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll)) {
              obj->name = std::string(rename_buf);
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
    }

    void scene_hierarchy_node::draw_object_tree(scene* active_scene, scene_object& object, uint32_t indent_level) {
      const auto child_ids = active_scene->get_children_ids(object.id);
      bool has_children = !child_ids.empty();
      bool is_expanded = expanded_ids.contains(object.id);

      float row_y = ImGui::GetCursorScreenPos().y;
      hierarchy::draw_indent_guide(indent_level, row_y, hierarchy::kItemHeight);

      hierarchy::item_flags flags{};
      flags.selected = (selected_object_id == object.id);
      flags.has_children = has_children;
      flags.expanded = is_expanded;
      flags.visible = object.visible;
      flags.is_scene_root = false;
      flags.disabled = !object.visible;
      flags.indent_level = indent_level;

      auto interaction = hierarchy::draw_item(object.name, object.id, flags);

#define DEBUG_DRAW 0
#if DEBUG_DRAW
      /// draw flags
      ImGui::Text(
        "Flags:\nflags.selected = %s\nflags.has_children = %s\nflags.expanded = %s\nflags.visible = %s\nflags.is_scene_root = %s\nflags.disabled = %s",
        flags.selected ? "true" : "false",
        flags.has_children ? "true" : "false",
        flags.expanded ? "true" : "false",
        flags.visible ? "true" : "false",
        flags.is_scene_root ? "true" : "false",
        flags.disabled ? "true" : "false"
      );
      /// draw interaction
      ImGui::Text(
        "Interaction:\nclicked = %s\ndouble_clicked = %s\nright_clicked = %s\nexpand_toggled = %s\nvisibility_toggled = %s",
        interaction.clicked ? "true" : "false",
        interaction.double_clicked ? "true" : "false",
        interaction.right_clicked ? "true" : "false",
        interaction.expand_toggled ? "true" : "false",
        interaction.visibility_toggled ? "true" : "false"
      );
#endif

      if (interaction.clicked) {
        select_object(object.id);
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
      }

      if (interaction.double_clicked) {
        /// initiate rename
        renaming_object_id = object.id;
        std::strncpy(rename_buf, object.name.c_str(), sizeof(rename_buf));
        rename_buf[sizeof(rename_buf) - 1] = '\0';
        ImGui::OpenPopup("##hier_rename");
      }

      if (interaction.right_clicked) {
        select_object(object.id);
        ImGui::OpenPopup("##hier_context");
      }

      if (has_children && is_expanded) {
        for (natural_t child_id : child_ids) {
          scene_object* child = active_scene->find_object(child_id);
          if (child == nullptr) continue;

          if (!passes_filter(active_scene, *child)) continue;

          draw_object_tree(active_scene, *child, indent_level + 1);
        }
      }
    }

    void scene_hierarchy_node::draw_context_menu(scene* active_scene) {
      ImGui::PushStyleColor(ImGuiCol_PopupBg, colors::rgba_to_imvec4(colors::kBG2));
      ImGui::PushStyleColor(ImGuiCol_Border, colors::rgba_to_imvec4(colors::kBorderSubtle));
      ImGui::PushStyleColor(ImGuiCol_Text, colors::rgba_to_imvec4(colors::kText));
      ImGui::PushStyleColor(ImGuiCol_HeaderHovered, colors::rgba_to_imvec4(colors::hierarchy::kItemHover));

      if (ImGui::BeginPopup("##hier_context")) {
        if (ImGui::MenuItem("Create Empty Object")) {
          /// \todo active_scene->create_object("New Object", selected_object_id);
        }

        ImGui::Separator();

        bool has_selection = (selected_object_id != 0);

        if (ImGui::MenuItem("Rename", "F2", false, has_selection)) {
          scene_object* obj = active_scene->find_object(selected_object_id);
          if (obj != nullptr) {
            renaming_object_id = selected_object_id;
            std::strncpy(rename_buf, obj->name.c_str(), sizeof(rename_buf));
            rename_buf[sizeof(rename_buf) - 1] = '\0';
            ImGui::OpenPopup("##hier_rename");
          }
        }

        if (ImGui::MenuItem("Duplicate", "Ctrl+D", false, has_selection)) {
          /// \todo active_scene->duplicate_object(selected_object_id);
        }

        if (ImGui::MenuItem("Delete", "Del", false, has_selection)) {
          /// \todo active_scene->delete_object(selected_object_id);
          /// selected_object_id = 0;
        }

        ImGui::Separator();

        if (ImGui::MenuItem("Focus in Viewport", "F", false, has_selection)) {
          /// \todo fire viewport focus event
          /// events().fire("ui.scene-hierarchy.focus-object", value(selected_object_id));
        }

        ImGui::EndPopup();
      }

      ImGui::PopStyleColor(4);
    }

    void scene_hierarchy_node::select_object(natural_t object_id) {
      selected_object_id = object_id;
      events().trigger_event("ui.scene-hierarchy.object-selected", value(object_id));
    }

  }  // namespace ui
}  // namespace other
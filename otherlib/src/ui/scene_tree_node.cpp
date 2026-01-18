/**
 * \file ui/scene_tree_ui_node.cpp
 **/
#include "ui/scene_tree_node.hpp"

#include <imgui/imgui.h>

#include "scene/scene.hpp"

#include "driver/driver.hpp"

namespace other {
  namespace ui {

    void scene_tree_node::on_render_node_body() {
      if (driver_ptr == nullptr) {
        ImGui::Text("Driver pointer is null.");
        return;
      }

      auto active_scene = driver_ptr->get_active_scene();
      if (active_scene == nullptr) {
        ImGui::Text("No active scene.");
        return;
      }

      scene_object& root = active_scene->root_object();
      auto entities = active_scene->get_children(&root);
      for (const auto& entity : entities) {
        render_scene_object(active_scene, entity);
      }
    }

    void scene_tree_node::render_scene_object(scene* active_scene, const scene_object* obj) {
      OTHER_ASSERT(active_scene != nullptr, "Active scene is null, cannot render.");
      OTHER_ASSERT(obj != nullptr, "Scene object is null, cannot render.");

      std::string invis_button_label = "##select-" + std::to_string(obj->id);
      bool node_open = ImGui::TreeNodeEx((void*)(uintptr_t)obj->id, ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_OpenOnArrow, "%s (ID: %llu)", obj->name.c_str(), obj->id);

      if (ImGui::IsItemClicked()) {
        if ((ImGui::GetMousePos().x - ImGui::GetItemRectMin().x) > ImGui::GetTreeNodeToLabelSpacing()) {
          events().trigger_event("ui.scene-hierarchy.object-selected", value(obj->id));
        }
      }

      if (node_open) {
        auto children = active_scene->get_children(obj);
        for (const auto& child : children) {
          render_scene_object(active_scene, child);
        }

        ImGui::TreePop();
      }
    }

  }  // namespace ui
}  // namespace other
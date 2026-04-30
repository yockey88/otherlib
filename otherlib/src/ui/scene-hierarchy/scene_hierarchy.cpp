/**
 * \file ui/scene-hierarchy/scene_hierarchy.cpp
 **/
#include "ui/scene-hierarchy/scene_hierarchy.hpp"

#include "renderer/ui/ui_node.hpp"

#include "driver/driver.hpp"
#include "ui/property_inspector_node.hpp"
#include "ui/scene-hierarchy/scene_hierarchy_node.hpp"

namespace other {
  namespace ui {

    scene_hierarchy::scene_hierarchy(event_system& events, driver* drvr)
        : ui_window(events, "Scene Hierarchy"), driver_ptr(drvr) {
      get_event_system().register_event("ui.scene-hierarchy.object-selected");
      get_event_system().add_listener("ui.scene-hierarchy.object-selected", [this](const value& data) {
        if (data.type() != value_type::UINT64) {
          CORE_LOG_ERROR("Invalid data type for ui.scene-hierarchy.object-selected event. Expected integer.");
          return;
        }

        natural_t obj_id = data;
        select_scene_object(obj_id);
      });

      hierarchy_node_id = add_node(make_scope<scene_hierarchy_node>(this, drvr));
      property_inspector_node_id = add_node(make_scope<property_inspector_node>(this, drvr));
    }

    void scene_hierarchy::on_post_render_nodes() {
      // if (selected_object_ids.empty()) {
      //   return;
      // }

      // if (multiple_selection_enabled && selected_object_ids.size() > 1) {
      //   ImGui::Text("Multiple Objects Selected:");
      //   for (const auto& obj_id : selected_object_ids) {
      //     ImGui::Text("- Object ID: %llu", obj_id);
      //   }
      // } else if (selected_object_ids.size() == 1) {
      //   ImGui::Text("Selected Object ID: %llu", selected_object_ids[0]);
      // }
    }

    void scene_hierarchy::select_scene_object(natural_t object_id) {
      if (!multiple_selection_enabled && !selected_object_ids.empty()) {
        selected_object_ids.clear();
      }
      selected_object_ids.push_back(object_id);
    }

  }  // namespace ui
}  // namespace other
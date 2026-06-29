/**
 * \file ui/scene-hierarchy/scene_hierarchy.cpp
 **/
#include "ui/scene-hierarchy/scene_hierarchy.hpp"

#include "driver/driver.hpp"
#include "ui/scene-hierarchy/scene_hierarchy_node.hpp"
#include "ui/ui_node.hpp"

namespace other {
  namespace ui {

    scene_hierarchy::scene_hierarchy(editor_context& ctx, event_system& events, driver* drvr)
        : ui_window(&events, "Scene Hierarchy"), driver_ptr(drvr), editor_ctx(ctx) {
      hierarchy_node_id = add_node(make_ref<scene_hierarchy_node>(ctx, this, drvr));
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

  }  // namespace ui
}  // namespace other
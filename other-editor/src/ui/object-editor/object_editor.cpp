/**
 * \file ui/object-editor/object_editor.cpp
 **/
#include "ui/object-editor/object_editor.hpp"

#include "driver/driver.hpp"
#include "ui/object-editor/property_inspector_node.hpp"

namespace other {
  namespace ui {

    object_editor::object_editor(editor_context& ctx, event_system& events, driver* drvr)
        : ui_window(&events, "Object Editor"), driver_ptr(drvr), editor_ctx(ctx) {
      get_event_system().register_event("ui.scene-hierarchy.object-selected");
      get_event_system().add_listener("ui.scene-hierarchy.object-selected", [this](const value& data) {
        if (data.type() != value_type::UINT64) {
          CORE_LOG_ERROR("Invalid data type for ui.scene-hierarchy.object-selected event. Expected integer.");
          return;
        }

        natural_t obj_id = data;
        select_scene_object(obj_id);
      });

      property_inspector_node_id = add_node(make_ref<property_inspector_node>(this, drvr));
    }

    void object_editor::select_scene_object(natural_t object_id) {
      if (!multiple_selection_enabled && !selected_object_ids.empty()) {
        selected_object_ids.clear();
      }
      selected_object_ids.push_back(object_id);
    }

  }  // namespace ui
}  // namespace other
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
      property_inspector_node_id = add_node(make_ref<property_inspector_node>(ctx, this, drvr));
    }

  }  // namespace ui
}  // namespace other
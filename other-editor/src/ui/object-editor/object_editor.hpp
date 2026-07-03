/**
 * \file ui/object-editor/object_editor.hpp
 **/
#ifndef OTHERLIB_UI_OBJECT_EDITOR_OBJECT_EDITOR_HPP
#define OTHERLIB_UI_OBJECT_EDITOR_OBJECT_EDITOR_HPP

#include "ui/ui_window.hpp"

#include "editor_context.hpp"

namespace other {

  class driver;

  namespace ui {

    class object_editor : public ui_window {
     public:
      object_editor(editor_context& ctx, event_system& events, driver* drvr);
      virtual ~object_editor() = default;

     private:
      editor_context& editor_ctx;

      natural_t property_inspector_node_id = 0;
    };

  }  // namespace ui

}  // namespace other

#endif  // OTHERLIB_UI_OBJECT_EDITOR_OBJECT_EDITOR_HPP
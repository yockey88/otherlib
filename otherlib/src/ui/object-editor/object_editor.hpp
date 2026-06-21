/**
 * \file ui/object-editor/object_editor.hpp
 **/
#ifndef OTHERLIB_UI_OBJECT_EDITOR_OBJECT_EDITOR_HPP
#define OTHERLIB_UI_OBJECT_EDITOR_OBJECT_EDITOR_HPP

#include "renderer/ui/ui_window.hpp"

namespace other {

  class driver;

  namespace ui {

    class object_editor : public ui_window {
     public:
      object_editor(event_system& events, driver* drvr);
      virtual ~object_editor() = default;

     private:
      driver* driver_ptr = nullptr;

      /// \todo: this should be stored in editor state
      bool multiple_selection_enabled = false;
      std::vector<natural_t> selected_object_ids;

      natural_t property_inspector_node_id = 0;

      void select_scene_object(natural_t object_id);
    };

  }  // namespace ui

}  // namespace other

#endif  // OTHERLIB_UI_OBJECT_EDITOR_OBJECT_EDITOR_HPP
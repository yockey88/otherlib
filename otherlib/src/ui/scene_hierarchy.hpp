/**
 * \file ui/scene_hierarchy.hpp
 **/
#ifndef OTHERLIB_UI_SCENE_HIERARCHY_HPP
#define OTHERLIB_UI_SCENE_HIERARCHY_HPP

#include <renderer/ui/ui_window.hpp>

namespace other {

  class driver;

  namespace ui {

    class scene_hierarchy : public ui_window {
     public:
      scene_hierarchy(event_system& events, driver* drvr);
      virtual ~scene_hierarchy() = default;

     private:
      bool multiple_selection_enabled = false;
      std::vector<natural_t> selected_object_ids;

      driver* driver_ptr = nullptr;
      natural_t hierarchy_node_id = 0;
      natural_t property_inspector_node_id = 0;

      void on_post_render_nodes() override;

      void select_scene_object(natural_t object_id);
    };

  }  // namespace ui
}  // namespace other

#endif  // OTHERLIB_UI_SCENE_HIERARCHY_HPP
/**
 * \file ui/scene-hierarchy/scene_hierarchy.hpp
 **/
#ifndef OTHERLIB_UI_SCENE_HIERARCHY_HPP
#define OTHERLIB_UI_SCENE_HIERARCHY_HPP

#include "ui/ui_window.hpp"

#include "editor_context.hpp"

namespace other {

  class driver;

  namespace ui {

    class scene_hierarchy : public ui_window {
     public:
      scene_hierarchy(editor_context& ctx, event_system& events, driver* drvr);
      virtual ~scene_hierarchy() = default;

      void on_render_body() override;

     private:
      editor_context& editor_ctx;
      driver* driver_ptr = nullptr;
      natural_t hierarchy_node_id = 0;

      constexpr inline static size_t kBufferSize = 256;

      char filter_buf[kBufferSize] = {};
      std::string filter_lower;

      natural_t renaming_object_id = 0;
      char rename_buf[kBufferSize] = {};

      char obj_creation_name_buf[kBufferSize] = {};

      void draw_object_context_menu(scene* active_scene, scene_object& object);
      void draw_hierarchy_context_menu(scene* active_scene);

      bool passes_filter(scene* active_scene, scene_object& object) const;
      void draw_object_tree(scene* active_scene, scene_object& object, uint32_t indent_level);

      void select_object(natural_t object_id);
    };

  }  // namespace ui
}  // namespace other

#endif  // OTHERLIB_UI_SCENE_HIERARCHY_HPP
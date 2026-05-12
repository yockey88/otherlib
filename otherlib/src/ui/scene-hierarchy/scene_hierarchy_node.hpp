/**
 * \file ui/scene-hierarchy/scene_hierarchy_node.hpp
 **/
#ifndef OTHERLIB_UI_SCENE_HIERARCHY_NODE_HPP
#define OTHERLIB_UI_SCENE_HIERARCHY_NODE_HPP

#include <string>
#include <unordered_map>
#include <unordered_set>

#include "renderer/ui/ui_node.hpp"

#include "ui/scene-hierarchy/hierarchy_widgets.hpp"

namespace other {

  class driver;
  class scene;
  struct scene_object;

  namespace ui {

    class scene_hierarchy_node : public ui_node {
     public:
      scene_hierarchy_node(ui_window* window, driver* driver);
      virtual ~scene_hierarchy_node() = default;

     private:
      driver* driver_ptr = nullptr;

      natural_t selected_object_id = 0;
      //  Expansion state  (set of expanded object ids)
      std::unordered_set<natural_t> expanded_ids;

      char filter_buf[256] = {};
      std::string filter_lower;

      natural_t renaming_object_id = 0;
      char rename_buf[256] = {};

      bool passes_filter(scene* active_scene, scene_object& object) const;
      void on_render_node_body() override;
      void draw_object_tree(scene* active_scene, scene_object& object, uint32_t indent_level);
      void draw_context_menu(scene* active_scene);

      void select_object(natural_t object_id);
    };

  }  // namespace ui
}  // namespace other

#endif  // OTHERLIB_UI_SCENE_HIERARCHY_NODE_HPP
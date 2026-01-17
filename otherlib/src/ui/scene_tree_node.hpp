/**
 * \file scene_tree_node.hpp
 **/
#ifndef OTHERLIB_UI_SCENE_TREE_NODE_HPP
#define OTHERLIB_UI_SCENE_TREE_NODE_HPP

#include "renderer/ui/ui_node.hpp"

namespace other {

  struct scene_object;

  class scene;
  class driver;

  namespace ui {

    struct scene_tree_node : public ui_node {
      scene_tree_node(ui_window* parent, driver* drvr)
          : ui_node(parent, "Hierarchy"), driver_ptr(drvr) {}
      virtual ~scene_tree_node() = default;

     private:
      driver* driver_ptr = nullptr;

      void on_render_node_body() override;

      void render_scene_object(scene* active_scene, const scene_object* obj);
    };

  }  // namespace ui
}  // namespace other

#endif  // OTHERLIB_UI_SCENE_TREE_NODE_HPP
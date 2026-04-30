/**
 * \file ui/asset-browser/asset_browser_tree_node.hpp
 **/
#ifndef OTHERLIB_UI_ASSET_BROWSER_TREE_NODE_HPP
#define OTHERLIB_UI_ASSET_BROWSER_TREE_NODE_HPP

#include "renderer/ui/ui_node.hpp"

#include "ui/asset-browser/asset_browser_widgets.hpp"

namespace other {

  class driver;

  namespace ui {

    class asset_browser_tree_node : public ui_node {
     public:
      asset_browser_tree_node(ui_window* windowm, driver* driver_ptr);
      ~asset_browser_tree_node() override = default;

      using navigate_fn = std::function<void(const std::string& path)>;
      void set_navigate_callback(navigate_fn fn);

      void set_selected_path(const std::string& path);
      void refresh_tree();

      /// called by content_browser::on_render_body() for manual layout
      void draw_content() { on_render_node_body(); }

     private:
      driver* driver_ptr = nullptr;

      std::vector<asset_browser_w::dir_tree_node> nodes;
      std::string selected_path = "assets";

      navigate_fn on_navigate;

      void rebuild_tree();

      void on_render_node_body() override;
    };

  }  // namespace ui
}  // namespace other

#endif  // OTHERLIB_UI_ASSET_BROWSER_TREE_NODE_HPP
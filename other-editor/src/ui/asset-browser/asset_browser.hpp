/**
 * \file ui/asset-browser/asset_browser.hpp
 **/
#ifndef OTHERLIB_UI_ASSET_BROWSER_HPP
#define OTHERLIB_UI_ASSET_BROWSER_HPP

#include "ui/asset-browser/asset_browser_grid_node.hpp"
#include "ui/asset-browser/asset_browser_tree_node.hpp"
#include "ui/ui_window.hpp"


namespace other {

  class driver;

  namespace ui {

    class asset_browser : public ui_window {
     public:
      asset_browser(event_system& events, driver* driver);
      ~asset_browser() override = default;

     private:
      float splitter_width = 180.f;

      scope<asset_browser_tree_node> tree_node;
      scope<asset_browser_grid_node> grid_node;

      void on_render_body() override;
    };

  }  // namespace ui
}  // namespace other

#endif  // OTHERLIB_UI_ASSET_BROWSER_HPP
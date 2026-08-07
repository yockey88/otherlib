/**
 * \file ui/asset-browser/asset_browser_grid_node.hpp
 **/
#ifndef OTHERLIB_UI_ASSET_BROWSER_GRID_NODE_HPP
#define OTHERLIB_UI_ASSET_BROWSER_GRID_NODE_HPP

#include "ui/asset-browser/asset_browser_widgets.hpp"
#include "data-structures/std_container.hpp"
#include "ui/ui_node.hpp"


namespace other {

  class driver;

  namespace ui {

    namespace abw = asset_browser_w;

    class asset_browser_grid_node : public ui_node {
     public:
      asset_browser_grid_node(ui_window* parent, driver* drvr);
      virtual ~asset_browser_grid_node() = default;

      void navigate_to(const std::string& path);
      void refresh_listing();

      inline const std::string& get_current_path() const { return current_path; }
      inline int selected_asset_index() const { return selected_asset_idx; }
      inline const ostd::vector<abw::asset_card_desc>& get_assets() const { return assets; }

      /// called by content_browser::on_render_body() for manual layout
      void draw_content() { on_render_node_body(); }

     private:
      driver* driver_ptr = nullptr;

      /// navigation state
      std::string current_path = "assets";
      ostd::vector<abw::breadcrumb_segment> breadcrumbs;

      /// search / filter
      char search_buf[256] = {};
      std::string search_lower;

      ostd::vector<abw::filter_pill_desc> filters;
      ostd::vector<abw::asset_card_desc> assets;
      int selected_asset_idx = -1;

      float zoom = 0.5f;

      /// frames since the last filesystem walk; starts stale so the first frame scans
      uint32_t frames_since_refresh = 1000;

      void rebuild_breadcrumbs();
      void rebuild_asset_list();
      bool passes_filter(const abw::asset_card_desc& desc) const;

      void on_render_node_body() override;
    };

  }  // namespace ui
}  // namespace other

#endif  // OTHERLIB_UI_ASSET_BROWSER_GRID_NODE_HPP
/**
 * \file ui/asset_browser_widgets.hpp
 **/
#ifndef OTHERLIB_UI_ASSET_BROWSER_WIDGETS_HPP
#define OTHERLIB_UI_ASSET_BROWSER_WIDGETS_HPP

#include <string>
#include <vector>

#include <glm/glm.hpp>
#include <imgui/imgui.h>

#include "asset/asset.hpp"

namespace other {
  namespace ui {
    namespace asset_browser_w {

      constexpr const char* kDragDropPayloadType = "OTHER_ASSET_DND";

      struct asset_drag_drop_payload {
        natural_t handler_asset_id = 0;
        asset::type asset_type = asset::EMPTY;
      };

      constexpr float kToolbarHeight = 34.f;
      constexpr float kFilterBarHeight = 28.f;
      constexpr float kStatusBarHeight = 22.f;
      constexpr float kDirTreeMinWidth = 120.f;
      constexpr float kDirTreeMaxWidth = 350.f;
      constexpr float kDirTreeDefaultWidth = 180.f;
      constexpr float kDirItemHeight = 22.f;
      constexpr float kIndentWidth = 12.f;
      constexpr float kCardSpacing = 8.f;
      constexpr float kCardMinWidth = 72.f;
      constexpr float kCardMaxWidth = 160.f;
      constexpr float kCardThumbRatio = 0.72f;
      constexpr float kCardInfoHeight = 28.f;
      constexpr float kCardRounding = 5.f;
      constexpr float kSigStripeWidth = 3.f;
      constexpr float kPaddingX = 10.f;
      constexpr float kPaddingY = 6.f;
      constexpr float kFilterPillHeight = 18.f;
      constexpr float kFilterDotRadius = 3.f;

      // necessary because asset::type has no equivalent for folders or other special files that the UI needs to represent
      enum class asset_type : uint8_t {
        FOLDER = 0,
        TEXTURE,
        MODEL_SOURCE,
        MODEL,
        ANIMATION,
        SCRIPT_SOURCE,
        SCRIPT,
        AUDIO,
        SCENE,
        SCENE_OBJECT,
        INPUT_MAP,
        PIPELINE,
        UNKNOWN,
      };

      struct breadcrumb_segment {
        std::string label;
        std::string full_path;
      };

      struct filter_pill_desc {
        const char* label;
        asset_type type;
        bool active;
      };

      struct dir_tree_node {
        std::string name;
        std::string path;
        int depth = 0;
        bool is_expanded = false;
        bool has_children = true;
      };

      struct asset_card_desc {
        std::string name;
        std::string meta;  ///< e.g. "2048×2048 · 4.2 MB"
        asset_type type = asset_type::UNKNOWN;

        ImTextureID thumbnail_id = 0;
        bool is_selected = false;

        natural_t handler_asset_id = 0;
        std::string asset_path;
      };

      struct status_bar_info {
        int total_assets = 0;
        int folder_count = 0;
        const char* selected_name = nullptr;
        float zoom_normalized = 0.5f;  ///< 0..1
      };

      glm::vec4 color_for_asset_type(asset_type type);
      const char* badge_for_asset_type(asset_type type);
      const char* icon_for_asset_type(asset_type type);
      asset::type map_ui_to_asset_type(asset_type type);

      float card_width_from_zoom(float zoom_normalized);

      int draw_breadcrumbs(const std::vector<breadcrumb_segment>& segments);
      bool draw_search_input(char* buf, uint32_t buf_size, float width);
      bool draw_filter_bar(std::vector<filter_pill_desc>& filters, int visible_asset_count);
      bool draw_dir_tree_item(dir_tree_node& node, bool is_selected);
      bool draw_asset_card(asset_card_desc& desc, float card_width);
      bool draw_status_bar(status_bar_info& info);

    }  // namespace asset_browser_w
  }  // namespace ui
}  // namespace other

#endif  // OTHERLIB_UI_ASSET_BROWSER_WIDGETS_HPP
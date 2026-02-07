/**
 * \file ui/hierarchy_widgets.hpp
 **/
#ifndef OTHERLIB_UI_HIERARCHY_WIDGETS_HPP
#define OTHERLIB_UI_HIERARCHY_WIDGETS_HPP

#include "core/defines.hpp"

namespace other {
  namespace ui {
    namespace hierarchy {

      constexpr float kItemHeight = 22.f;
      constexpr float kSearchBarHeight = 28.f;
      constexpr float kSearchBarPadding = 8.f;
      constexpr float kIndentWidth = 16.f;
      constexpr float kArrowWidth = 12.f;
      constexpr float kVisibilityIconWidth = 18.f;
      constexpr float kItemPaddingX = 12.f;
      constexpr float kItemGap = 6.f;

      // ═══════════════════════════════════════════════════════════════════════
      //  Search / Filter Bar
      //
      //  ┌──────────────────────────────────────────┐
      //  │  🔍 filter objects...                     │
      //  └──────────────────────────────────────────┘
      //
      //  Returns true if the filter text changed.
      // ═══════════════════════════════════════════════════════════════════════
      bool draw_search_bar(char* filter_buf, uint32_t buf_size);

      // ═══════════════════════════════════════════════════════════════════════
      //  Hierarchy Item
      //
      //  ┌──────────────────────────────────────────┐
      //  │  ▾  ObjectName                        👁  │
      //  └──────────────────────────────────────────┘
      //
      //  Draws a single row in the hierarchy tree.
      // ═══════════════════════════════════════════════════════════════════════

      struct item_flags {
        bool selected = false;       ///< item is the current selection
        bool has_children = false;   ///< show expand arrow
        bool expanded = false;       ///< arrow points down (▾) vs right (▸)
        bool visible = true;         ///< eye icon state
        bool is_scene_root = false;  ///< renders with scene icon styling
        bool disabled = false;       ///< grayed-out text
        uint32_t indent_level = 0;   ///< nesting depth (0 = root level)
      };

      /// result of drawing a hierarchy item — tells the caller what happened
      struct item_interaction {
        bool clicked = false;             ///< row was clicked (select this item)
        bool expand_toggled = false;      ///< expand arrow was clicked
        bool visibility_toggled = false;  ///< eye icon was clicked
        bool double_clicked = false;      ///< row was double-clicked (rename / focus)
        bool right_clicked = false;       ///< right-click context menu
      };

      item_interaction draw_item(const std::string_view label, natural_t object_id, item_flags flags);

      void draw_indent_guide(uint32_t indent_level, float row_top_y, float row_height);
      void draw_drop_target_line(float y_position, float indent_offset = 0.f);
      void draw_empty_scene_message();

    }  // namespace hierarchy
  }  // namespace ui
}  // namespace other

#endif  // OTHERLIB_UI_HIERARCHY_WIDGETS_HPP
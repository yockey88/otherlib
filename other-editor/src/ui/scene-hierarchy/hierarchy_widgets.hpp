/**
 * \file ui/scene-hierarchy/hierarchy_widgets.hpp
 **/
#ifndef OTHERLIB_UI_HIERARCHY_WIDGETS_HPP
#define OTHERLIB_UI_HIERARCHY_WIDGETS_HPP

#include "core/defines.hpp"

namespace other {
  namespace ui {
    namespace hierarchy {

      constexpr float kItemHeight = 15.f;
      constexpr float kSearchBarHeight = 28.f;
      constexpr float kSearchBarPadding = 8.f;
      constexpr float kIndentWidth = 8.f;
      constexpr float kArrowWidth = 12.f;
      constexpr float kVisibilityIconWidth = 18.f;
      constexpr float kItemPaddingX = 10.f;
      constexpr float kItemGap = 6.f;

      struct item_flags {
        bool selected = false;
        bool has_children = false;
        bool expanded = false;  /// arrow points down (▾) vs right (▸)
        bool visible = true;
        bool disabled = false;
        uint32_t indent_level = 0;
      };

      struct item_interaction {
        bool clicked = false;
        bool expand_toggled = false;
        bool visibility_toggled = false;
        bool double_clicked = false;
        bool right_clicked = false;
      };

      bool draw_search_bar(char* filter_buf, uint32_t buf_size);
      item_interaction draw_item(const std::string_view label, natural_t object_id, item_flags flags);

      void draw_indent_guide(uint32_t indent_level, float row_top_y, float row_height);
      void draw_drop_target_line(float y_position, float indent_offset = 0.f);
      void draw_empty_scene_message();

    }  // namespace hierarchy
  }  // namespace ui
}  // namespace other

#endif  // OTHERLIB_UI_HIERARCHY_WIDGETS_HPP
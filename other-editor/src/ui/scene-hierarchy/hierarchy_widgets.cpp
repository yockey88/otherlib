/**
 * \file ui/scene-hierarchy/hierarchy_widgets.cpp
 **/
#include "ui/scene-hierarchy/hierarchy_widgets.hpp"

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

#include "theme/colors.hpp"

namespace other {
  namespace ui {
    namespace hierarchy {

      bool draw_search_bar(char* filter_buf, uint32_t buf_size) {
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8.f, 5.f));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.f);
        ImGui::PushStyleColor(ImGuiCol_FrameBg, colors::rgba_to_imvec4(colors::hierarchy::kSearchBG));
        ImGui::PushStyleColor(ImGuiCol_Border, colors::rgba_to_imvec4(colors::hierarchy::kSearchBorder));
        ImGui::PushStyleColor(ImGuiCol_Text, colors::rgba_to_imvec4(colors::kText));

        ImGuiContext& g = *GImGui;
        bool was_active = (g.ActiveId == ImGui::GetID("##hier_filter"));
        if (was_active) {
          ImGui::PushStyleColor(ImGuiCol_Border, colors::rgba_to_imvec4(colors::hierarchy::kSearchBorderFocused));
        }

        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + kSearchBarPadding);
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + kSearchBarPadding);
        float search_w = ImGui::GetContentRegionAvail().x - kSearchBarPadding * 2.f;
        ImGui::PushItemWidth(search_w);

        bool changed = ImGui::InputTextWithHint(
          "##hier_filter", "filter objects...", filter_buf, buf_size);

        ImGui::PopItemWidth();

        if (was_active) {
          ImGui::PopStyleColor();  // focused border
        }

        ImGui::PopStyleColor(3);
        ImGui::PopStyleVar(2);

        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 4.f);

        return changed;
      }

      item_interaction draw_item(const std::string_view label, natural_t object_id, item_flags flags) {
        item_interaction result{};
        ImDrawList* dl = ImGui::GetWindowDrawList();

        const ImVec2 cursor = ImGui::GetCursorScreenPos();
        const float avail_w = ImGui::GetContentRegionAvail().x;
        const float indent = static_cast<float>(flags.indent_level) * kIndentWidth;

        ImVec2 row_min = cursor;
        ImVec2 row_max = { cursor.x + avail_w, cursor.y + kItemHeight };

        ImGui::PushID(static_cast<int>(object_id));

        /// invisible button for the full row to capture interactions
        ImGui::SetCursorScreenPos(row_min);
        ImGui::InvisibleButton("##hier_row", ImVec2(avail_w, kItemHeight));

        bool hovered = ImGui::IsItemHovered();
        result.clicked = ImGui::IsItemClicked(ImGuiMouseButton_Left);
        result.double_clicked = ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left);
        result.right_clicked = ImGui::IsItemClicked(ImGuiMouseButton_Right);

        if (flags.selected) {
          dl->AddRectFilled(row_min, row_max, colors::to_im_col(colors::hierarchy::kItemSelected));
        } else if (hovered) {
          dl->AddRectFilled(row_min, row_max, colors::to_im_col(colors::hierarchy::kItemHover));
        }

        // ─────────────────────────────────────────────────────────
        //  Expand Arrow
        // ─────────────────────────────────────────────────────────
        float content_x = row_min.x + kItemPaddingX + indent;
        float text_y = row_min.y + (kItemHeight - ImGui::GetFontSize()) * 0.5f;

        if (flags.has_children) {
          /// detect click on arrow region specifically
          ImVec2 arrow_min = { content_x, row_min.y };
          ImVec2 arrow_max = { content_x + kArrowWidth, row_max.y };

          bool arrow_hovered = ImGui::IsMouseHoveringRect(arrow_min, arrow_max);
          ImU32 arrow_col = arrow_hovered ? colors::to_im_col(colors::hierarchy::kExpandArrowHover) : colors::to_im_col(colors::hierarchy::kExpandArrow);

          if (arrow_hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            result.expand_toggled = true;
            result.clicked = false;  // don't also select when toggling expand
          }

          /// draw the triangle
          const float arrow_size = 5.f;
          float cx = content_x + kArrowWidth * 0.5f;
          float cy = row_min.y + kItemHeight * 0.5f;

          if (flags.expanded) {
            /// ▾ pointing down
            dl->AddTriangleFilled(
              { cx - arrow_size, cy - arrow_size * 0.4f },
              { cx + arrow_size, cy - arrow_size * 0.4f },
              { cx, cy + arrow_size * 0.6f },
              arrow_col);
          } else {
            /// ▸ pointing right
            dl->AddTriangleFilled(
              { cx - arrow_size * 0.4f, cy - arrow_size },
              { cx + arrow_size * 0.6f, cy },
              { cx - arrow_size * 0.4f, cy + arrow_size },
              arrow_col);
          }
        }

        content_x += kArrowWidth + kItemGap;

        // ─────────────────────────────────────────────────────────
        //  Scene Root Icon (optional)
        // ─────────────────────────────────────────────────────────
        if (flags.is_scene_root) {
          const float icon_size = 12.f;
          float icon_y = row_min.y + (kItemHeight - icon_size) * 0.5f;

          /// small filled circle as scene icon
          dl->AddCircleFilled(
            { content_x + icon_size * 0.5f, icon_y + icon_size * 0.5f },
            icon_size * 0.45f,
            colors::to_im_col(colors::scene_object::kSignature));
          content_x += icon_size + kItemGap;
        }

        // ─────────────────────────────────────────────────────────
        //  Label Text
        // ─────────────────────────────────────────────────────────
        ImU32 text_col;
        if (flags.disabled) {
          text_col = colors::to_im_col(colors::hierarchy::kItemTextDisabled);
        } else if (flags.selected) {
          text_col = colors::to_im_col(colors::hierarchy::kItemTextSelected);
        } else {
          text_col = colors::to_im_col(colors::hierarchy::kItemText);
        }

        /// clip label so it doesn't bleed into the visibility icon
        float label_max_x = row_max.x - kItemPaddingX - kVisibilityIconWidth;
        ImGui::PushClipRect(
          { content_x, row_min.y },
          { label_max_x, row_max.y },
          true);
        dl->AddText({ content_x, text_y }, text_col, label.data(), label.data() + label.size());
        ImGui::PopClipRect();

        // ─────────────────────────────────────────────────────────
        //  Visibility Icon (eye)
        // ─────────────────────────────────────────────────────────
        {
          float icon_x = row_max.x - kItemPaddingX - kVisibilityIconWidth;
          ImVec2 icon_min = { icon_x, row_min.y };
          ImVec2 icon_max = { icon_x + kVisibilityIconWidth, row_max.y };

          bool icon_hovered = ImGui::IsMouseHoveringRect(icon_min, icon_max);

          if (icon_hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            result.visibility_toggled = true;
            result.clicked = false;  // don't also select
          }

          /// draw a simple eye glyph using draw primitives
          float ecx = icon_x + kVisibilityIconWidth * 0.5f;
          float ecy = row_min.y + kItemHeight * 0.5f;

          ImU32 eye_col = flags.visible ? colors::to_im_col(colors::hierarchy::kIconVisible) : colors::to_im_col(colors::hierarchy::kIconHidden);

          if (flags.visible) {
            /// eye shape: two arcs forming an almond + filled pupil
            const float ew = 5.f;   // half-width of eye
            const float eh = 2.5f;  // half-height of eye

            /// top arc
            dl->PathArcTo({ ecx, ecy + eh * 1.2f }, ew * 1.4f, -2.3f, -0.84f, 8);
            /// bottom arc
            dl->PathArcTo({ ecx, ecy - eh * 1.2f }, ew * 1.4f, 0.84f, 2.3f, 8);
            dl->PathStroke(eye_col, ImDrawFlags_Closed, 1.2f);

            /// pupil
            dl->AddCircleFilled({ ecx, ecy }, 1.8f, eye_col);
          } else {
            /// hidden: just a dash
            dl->AddLine(
              { ecx - 4.f, ecy },
              { ecx + 4.f, ecy },
              eye_col, 1.2f);
          }
        }

        ImGui::PopID();

        /// advance cursor past this row
        ImGui::SetCursorScreenPos({ row_min.x, row_max.y });

        return result;
      }

      void draw_indent_guide(uint32_t indent_level, float row_top_y, float row_height) {
        if (indent_level == 0) {
          return;
        }

        ImDrawList* dl = ImGui::GetWindowDrawList();
        const float base_x = ImGui::GetWindowPos().x + kItemPaddingX;

        for (uint32_t i = 1; i <= indent_level; ++i) {
          float x = base_x + static_cast<float>(i) * kIndentWidth + kArrowWidth * 0.5f;
          dl->AddLine({ x, row_top_y }, { x, row_top_y + row_height }, colors::to_im_col(colors::hierarchy::kIndentGuide), 1.f);
        }
      }

      void draw_drop_target_line(float y_position, float indent_offset) {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        float x_start = ImGui::GetWindowPos().x + kItemPaddingX + indent_offset;
        float x_end = ImGui::GetWindowPos().x + ImGui::GetWindowSize().x - kItemPaddingX;

        dl->AddLine({ x_start, y_position }, { x_end, y_position }, colors::to_im_col(colors::hierarchy::kDropTargetLine), 2.f);
        dl->AddCircleFilled({ x_start, y_position }, 3.f, colors::to_im_col(colors::hierarchy::kDropTargetLine));
      }

      void draw_empty_scene_message() {
        float avail_w = ImGui::GetContentRegionAvail().x;
        float avail_h = ImGui::GetContentRegionAvail().y;

        const char* msg = "No active scene";
        ImVec2 text_size = ImGui::CalcTextSize(msg);

        float x = ImGui::GetCursorScreenPos().x + (avail_w - text_size.x) * 0.5f;
        float y = ImGui::GetCursorScreenPos().y + avail_h * 0.35f;

        ImGui::GetWindowDrawList()->AddText({ x, y }, colors::to_im_col(colors::kTextMuted), msg);
      }

    }  // namespace hierarchy
  }  // namespace ui
}  // namespace other
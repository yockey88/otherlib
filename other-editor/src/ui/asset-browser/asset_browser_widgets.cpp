/**
 * \file ui/asset-browser/asset_browser_widgets.cpp
 **/
#include "asset_browser_widgets.hpp"

#include <cstring>
#include <format>

#include "theme/colors.hpp"

namespace other {
  namespace ui {
    namespace asset_browser_w {

      glm::vec4 color_for_asset_type(asset_type type) {
        using namespace colors::asset;
        switch (type) {
          case asset_type::FOLDER: return kFolder;
          case asset_type::TEXTURE: return kTexture;
          case asset_type::MODEL_SOURCE: return kModelSource;
          case asset_type::MODEL: return kModel;
          case asset_type::ANIMATION: return kAnimation;
          case asset_type::SCRIPT_SOURCE: return kScriptSource;
          case asset_type::SCRIPT: return kScript;
          case asset_type::AUDIO: return kAudio;
          case asset_type::SCENE: return kScene;
          default: return kUnknown;
        }
      }

      const char* badge_for_asset_type(asset_type type) {
        switch (type) {
          case asset_type::FOLDER: return "DIR";
          case asset_type::TEXTURE: return "TEX";
          case asset_type::MODEL_SOURCE: return "FBX";
          case asset_type::MODEL: return "MDL";
          case asset_type::ANIMATION: return "ANIM";
          case asset_type::SCRIPT_SOURCE: return "LUA";
          case asset_type::SCRIPT: return "SCR";
          case asset_type::AUDIO: return "WAV";
          case asset_type::SCENE: return "SCN";
          default: return "???";
        }
      }

      const char* icon_for_asset_type(asset_type type) {
        switch (type) {
          case asset_type::FOLDER: return "D";
          case asset_type::TEXTURE: return "T";
          case asset_type::MODEL_SOURCE: return "Ms";
          case asset_type::MODEL: return "M";
          case asset_type::ANIMATION: return "An";
          case asset_type::SCRIPT_SOURCE: return "Ls";
          case asset_type::SCRIPT: return "S";
          case asset_type::AUDIO: return "Au";
          case asset_type::SCENE: return "Sc";
          // case asset_type::SCENE_OBJECT: return "So";
          default: return "?";
        }
      }

      float card_width_from_zoom(float zoom_normalized) {
        return kCardMinWidth + zoom_normalized * (kCardMaxWidth - kCardMinWidth);
      }

      asset::type map_ui_to_asset_type(asset_type type) {
        switch (type) {
          case asset_type::TEXTURE: return asset::TEXTURE;
          case asset_type::MODEL_SOURCE: return asset::MODEL_SOURCE;
          case asset_type::MODEL: return asset::MODEL;
          case asset_type::ANIMATION: return asset::ANIMATION;
          case asset_type::SCRIPT_SOURCE: return asset::SCRIPT_SOURCE;
          case asset_type::SCRIPT: return asset::SCRIPT;
          case asset_type::AUDIO: return asset::AUDIO;
          case asset_type::SCENE: return asset::SCENE;
          default: return asset::EMPTY;
        }
      }

      int draw_breadcrumbs(const std::vector<breadcrumb_segment>& segments) {
        using namespace colors::asset_browser;
        int clicked = -1;
        ImDrawList* dl = ImGui::GetWindowDrawList();

        for (size_t i = 0; i < segments.size(); ++i) {
          const auto& seg = segments[i];
          bool is_last = (i == segments.size() - 1);

          /// separator
          if (i > 0) {
            ImGui::SameLine(0.f, 2.f);
            ImGui::PushStyleColor(ImGuiCol_Text, colors::rgba_to_imvec4(kBreadcrumbSeparator));
            ImGui::TextUnformatted(">");
            ImGui::PopStyleColor();
            ImGui::SameLine(0.f, 2.f);
          }

          if (is_last) {
            /// current segment — not clickable
            ImGui::PushStyleColor(ImGuiCol_Text, colors::rgba_to_imvec4(kBreadcrumbCurrent));
            ImGui::TextUnformatted(seg.label.c_str());
            ImGui::PopStyleColor();
          } else {
            /// clickable segment
            ImGui::PushID(static_cast<int>(i));
            ImVec2 text_size = ImGui::CalcTextSize(seg.label.c_str());
            ImVec2 cursor = ImGui::GetCursorScreenPos();
            float btn_w = text_size.x + 8.f;
            float btn_h = text_size.y + 4.f;

            if (ImGui::InvisibleButton("##crumb", ImVec2(btn_w, btn_h))) {
              clicked = static_cast<int>(i);
            }
            bool hovered = ImGui::IsItemHovered();

            if (hovered) {
              dl->AddRectFilled(cursor, { cursor.x + btn_w, cursor.y + btn_h }, IM_COL32(255, 255, 255, 10), 3.f);
            }

            glm::vec4 text_col = hovered ? kBreadcrumbCurrent : kBreadcrumbText;
            dl->AddText({ cursor.x + 4.f, cursor.y + 2.f }, colors::to_im_col(text_col), seg.label.c_str());

            ImGui::PopID();
          }
        }

        return clicked;
      }

      bool draw_search_input(char* buf, uint32_t buf_size, float width) {
        using namespace colors::asset_browser;

        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.f);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8.f, 3.f));
        ImGui::PushStyleColor(ImGuiCol_FrameBg, colors::rgba_to_imvec4(kSearchFieldBG));
        ImGui::PushStyleColor(ImGuiCol_Border, colors::rgba_to_imvec4(kSearchFieldBorder));
        ImGui::PushStyleColor(ImGuiCol_Text, colors::rgba_to_imvec4(kFileName));

        ImGui::PushItemWidth(width);
        bool changed = ImGui::InputTextWithHint("##cb_search", ":filter assets...", buf, buf_size);
        ImGui::PopItemWidth();

        ImGui::PopStyleColor(3);
        ImGui::PopStyleVar(2);
        return changed;
      }

      bool draw_filter_bar(std::vector<filter_pill_desc>& filters, int visible_asset_count) {
        using namespace colors::asset_browser;
        bool changed = false;
        ImDrawList* dl = ImGui::GetWindowDrawList();

        for (size_t i = 0; i < filters.size(); ++i) {
          auto& f = filters[i];
          glm::vec4 sig = color_for_asset_type(f.type);
          if (f.type == asset_type::UNKNOWN) {
            sig = colors::kTextMuted;  /// "All" pill uses muted text
          }

          ImGui::PushID(static_cast<int>(i));

          ImVec2 label_size = ImGui::CalcTextSize(f.label);
          float pill_w = label_size.x + 22.f;

          ImVec2 cursor = ImGui::GetCursorScreenPos();
          ImVec2 pill_min = cursor;
          ImVec2 pill_max = { cursor.x + pill_w, cursor.y + kFilterPillHeight };

          if (ImGui::InvisibleButton("##pill", ImVec2(pill_w, kFilterPillHeight))) {
            f.active = !f.active;
            changed = true;
          }
          bool hovered = ImGui::IsItemHovered();

          /// hover background
          if (hovered) {
            dl->AddRectFilled(pill_min, pill_max, IM_COL32(255, 255, 255, 10), 9.f);
          }

          /// dot
          ImVec2 dot_center = { pill_min.x + 9.f, pill_min.y + kFilterPillHeight * 0.5f };
          if (f.active) {
            dl->AddCircleFilled(dot_center, kFilterDotRadius, colors::to_im_col(sig));
            dl->AddCircle(dot_center, kFilterDotRadius + 2.f, colors::to_im_col(glm::vec4(sig.r, sig.g, sig.b, 0.25f)));
          } else {
            dl->AddCircle(dot_center, kFilterDotRadius, colors::to_im_col(colors::kTextMuted));
          }

          /// label
          glm::vec4 text_col = f.active ? colors::mute_by_factor(sig, 0.5f) : (hovered ? kFilterHover : kFilterInactive);
          dl->AddText({ pill_min.x + 18.f, pill_min.y + (kFilterPillHeight - label_size.y) * 0.5f }, colors::to_im_col(text_col), f.label);

          ImGui::PopID();
          ImGui::SameLine(0.f, 4.f);
        }

        /// asset count on the right
        ImGui::SameLine(0.f, 0.f);
        float avail = ImGui::GetContentRegionAvail().x;
        std::string count_str = std::format("{} assets", visible_asset_count);
        ImVec2 count_size = ImGui::CalcTextSize(count_str.c_str());
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + avail - count_size.x - 4.f);
        ImGui::PushStyleColor(ImGuiCol_Text, colors::rgba_to_imvec4(kStatusText));
        ImGui::TextUnformatted(count_str.c_str());
        ImGui::PopStyleColor();

        return changed;
      }

      bool draw_dir_tree_item(dir_tree_node& node, bool is_selected) {
        using namespace colors::asset_browser;
        ImDrawList* dl = ImGui::GetWindowDrawList();

        float indent = kPaddingX + static_cast<float>(node.depth) * kIndentWidth;
        float full_w = ImGui::GetContentRegionAvail().x;

        ImVec2 cursor = ImGui::GetCursorScreenPos();
        ImVec2 row_min = cursor;
        ImVec2 row_max = { cursor.x + full_w, cursor.y + kDirItemHeight };

        ImGui::PushID(node.path.c_str());
        bool clicked = ImGui::InvisibleButton("##dir", ImVec2(full_w, kDirItemHeight));
        bool hovered = ImGui::IsItemHovered();
        ImGui::PopID();

        /// background
        if (is_selected) {
          dl->AddRectFilled(row_min, row_max, colors::to_im_col(kDirItemSelected));
        } else if (hovered) {
          dl->AddRectFilled(row_min, row_max, colors::to_im_col(kDirItemHover));
        }

        float x = row_min.x + indent;
        float text_y = row_min.y + (kDirItemHeight - ImGui::GetFontSize()) * 0.5f;

        /// expand arrow
        if (node.has_children) {
          const char* arrow = node.is_expanded ? "v" : ">";
          dl->AddText({ x, text_y }, IM_COL32(140, 140, 140, 255), arrow);
        }
        x += 14.f;

        /// folder icon
        dl->AddText({ x, text_y }, colors::to_im_col(colors::asset::kFolder), "D");
        x += 14.f;

        /// name
        glm::vec4 name_col = is_selected ?
          kBreadcrumbCurrent :
          kBreadcrumbText;
        dl->AddText({ x, text_y }, colors::to_im_col(name_col), node.name.c_str());

        if (clicked && node.has_children) {
          node.is_expanded = !node.is_expanded;
        }

        return clicked;
      }

      bool draw_asset_card(asset_card_desc& desc, float card_width) {
        using namespace colors::asset_browser;
        ImDrawList* dl = ImGui::GetWindowDrawList();

        float thumb_h = card_width * kCardThumbRatio;
        float card_h = thumb_h + kCardInfoHeight;

        ImVec2 cursor = ImGui::GetCursorScreenPos();
        ImVec2 card_min = cursor;
        ImVec2 card_max = { cursor.x + card_width, cursor.y + card_h };

        ImGui::PushID(desc.name.c_str());
        bool clicked = ImGui::InvisibleButton("##card", ImVec2(card_width, card_h));
        bool hovered = ImGui::IsItemHovered();

        if (desc.type != asset_type::FOLDER &&
            ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
          asset_drag_drop_payload payload{};
          payload.handler_asset_id = desc.handler_asset_id;
          payload.asset_type = map_ui_to_asset_type(desc.type);

          bool _ = ImGui::SetDragDropPayload(kDragDropPayloadType, &payload, sizeof(payload));

          glm::vec4 sig = color_for_asset_type(desc.type);
          ImGui::TextColored(ImVec4(sig.r, sig.g, sig.b, sig.a), "%s", badge_for_asset_type(desc.type));
          ImGui::SameLine();
          ImGui::TextUnformatted(desc.name.c_str());

          ImGui::EndDragDropSource();
        }

        ImGui::PopID();

        if (clicked) {
          desc.is_selected = true;
        }

        /// card background
        glm::vec4 bg_col = kGridCardBG;
        if (desc.is_selected)
          bg_col = kGridCardSelected;
        else if (hovered)
          bg_col = kGridCardHover;

        glm::vec4 border_col = desc.is_selected ? kGridCardBorderSelected : kGridCardBorder;

        dl->AddRectFilled(card_min, card_max, colors::to_im_col(bg_col), kCardRounding);
        dl->AddRect(card_min, card_max, colors::to_im_col(border_col), kCardRounding, 0, 1.f);

        /// thumbnail area
        ImVec2 thumb_min = card_min;
        ImVec2 thumb_max = { card_min.x + card_width, card_min.y + thumb_h };

        dl->PushClipRect(thumb_min, thumb_max, true);
        dl->AddRectFilled(thumb_min, thumb_max, colors::to_im_col(kThumbnailBG), kCardRounding, ImDrawFlags_RoundCornersTop);

        if (desc.thumbnail_id) {
          dl->AddImage(desc.thumbnail_id, thumb_min, thumb_max);
        } else {
          /// placeholder icon centered
          glm::vec4 icon_col = color_for_asset_type(desc.type);
          icon_col.a = 0.5f;
          const char* icon = icon_for_asset_type(desc.type);
          ImVec2 icon_size = ImGui::CalcTextSize(icon);
          dl->AddText(
            { thumb_min.x + (card_width - icon_size.x) * 0.5f, thumb_min.y + (thumb_h - icon_size.y) * 0.5f },
            colors::to_im_col(icon_col), icon);
        }

        /// signature stripe on left edge
        if (desc.type != asset_type::FOLDER) {
          glm::vec4 sig = color_for_asset_type(desc.type);
          dl->AddRectFilled(thumb_min, { thumb_min.x + kSigStripeWidth, thumb_max.y }, colors::to_im_col(sig));
        }

        /// type badge (bottom-right of thumbnail)
        if (desc.type != asset_type::FOLDER) {
          const char* badge = badge_for_asset_type(desc.type);
          ImVec2 badge_size = ImGui::CalcTextSize(badge);
          float pad_x = 4.f, pad_y = 2.f;
          ImVec2 badge_min = { thumb_max.x - badge_size.x - pad_x * 2 - 3.f, thumb_max.y - badge_size.y - pad_y * 2 - 3.f };
          ImVec2 badge_max = { badge_min.x + badge_size.x + pad_x * 2, badge_min.y + badge_size.y + pad_y * 2 };

          dl->AddRectFilled(badge_min, badge_max, IM_COL32(0, 0, 0, 180), 2.f);
          dl->AddText(
            { badge_min.x + pad_x, badge_min.y + pad_y },
            colors::to_im_col(color_for_asset_type(desc.type)), badge);
        }

        dl->PopClipRect();

        /// separator line
        dl->AddLine({ card_min.x, thumb_max.y }, { card_max.x, thumb_max.y }, colors::to_im_col(kBorder), 0.5f);

        /// info area — filename
        float text_x = card_min.x + 6.f;
        float text_y = thumb_max.y + 4.f;
        float max_text_w = card_width - 12.f;

        glm::vec4 name_col = desc.is_selected ? kFileNameSelected : kFileName;
        dl->PushClipRect({ text_x, text_y }, { text_x + max_text_w, text_y + ImGui::GetFontSize() }, true);
        dl->AddText({ text_x, text_y }, colors::to_im_col(name_col), desc.name.c_str());
        dl->PopClipRect();

        /// info area — meta line
        if (!desc.meta.empty()) {
          float meta_y = text_y + ImGui::GetFontSize() + 1.f;
          dl->PushClipRect({ text_x, meta_y }, { text_x + max_text_w, meta_y + ImGui::GetFontSize() }, true);
          dl->AddText({ text_x, meta_y }, colors::to_im_col(kStatusText), desc.meta.c_str());
          dl->PopClipRect();
        }

        return clicked;
      }

      bool draw_status_bar(status_bar_info& info) {
        using namespace colors::asset_browser;
        bool zoom_changed = false;
        ImDrawList* dl = ImGui::GetWindowDrawList();

        ImVec2 cursor = ImGui::GetCursorScreenPos();
        float full_w = ImGui::GetContentRegionAvail().x;
        ImVec2 bar_min = cursor;
        ImVec2 bar_max = { cursor.x + full_w, cursor.y + kStatusBarHeight };

        /// background + top border
        dl->AddRectFilled(bar_min, bar_max, colors::to_im_col(kStatusBarBG));
        dl->AddLine(bar_min, { bar_max.x, bar_min.y }, colors::to_im_col(kBorder), 1.f);

        float x = bar_min.x + kPaddingX;
        float text_y = bar_min.y + (kStatusBarHeight - ImGui::GetFontSize()) * 0.5f;

        /// asset count
        std::string num_str = std::format("{}", info.total_assets);
        ImVec2 num_size = ImGui::CalcTextSize(num_str.c_str());
        dl->AddText({ x, text_y }, colors::to_im_col(colors::kAccent), num_str.c_str());
        x += num_size.x;
        dl->AddText({ x, text_y }, colors::to_im_col(kStatusText), " assets");
        x += ImGui::CalcTextSize(" assets").x + 12.f;

        /// separator
        dl->AddLine({ x, bar_min.y + 5.f }, { x, bar_max.y - 5.f }, colors::to_im_col(kBorder));
        x += 12.f;

        /// folder count
        std::string folder_str = std::format("{} folders", info.folder_count);
        dl->AddText({ x, text_y }, colors::to_im_col(kStatusText), folder_str.c_str());
        x += ImGui::CalcTextSize(folder_str.c_str()).x + 12.f;

        /// selected asset name
        if (info.selected_name) {
          dl->AddLine({ x, bar_min.y + 5.f }, { x, bar_max.y - 5.f }, colors::to_im_col(kBorder));
          x += 12.f;
          dl->AddText({ x, text_y }, colors::to_im_col(kStatusText), "selected: ");
          x += ImGui::CalcTextSize("selected: ").x;
          dl->AddText({ x, text_y }, colors::to_im_col(colors::kAccent), info.selected_name);
        }

        /// zoom slider on far right
        float slider_w = 60.f;
        float slider_area_w = slider_w + 30.f;
        float slider_x = bar_max.x - slider_area_w - kPaddingX;

        dl->AddText({ slider_x, text_y }, colors::to_im_col(kStatusText), "-");
        slider_x += 12.f;

        float track_y = bar_min.y + kStatusBarHeight * 0.5f;
        ImVec2 track_min = { slider_x, track_y - 1.5f };
        ImVec2 track_max = { slider_x + slider_w, track_y + 1.5f };
        dl->AddRectFilled(track_min, track_max, IM_COL32(50, 50, 50, 255), 2.f);

        float fill_x = slider_x + slider_w * info.zoom_normalized;
        dl->AddRectFilled(track_min, { fill_x, track_max.y }, colors::to_im_col(colors::kAccent), 2.f);
        dl->AddCircleFilled({ fill_x, track_y }, 4.f, colors::to_im_col(colors::kAccent));

        dl->AddText({ slider_x + slider_w + 6.f, text_y }, colors::to_im_col(kStatusText), "+");

        /// invisible drag area for zoom slider
        ImGui::SetCursorScreenPos({ slider_x, bar_min.y });
        ImGui::InvisibleButton("##zoom_drag", ImVec2(slider_w, kStatusBarHeight));
        if (ImGui::IsItemActive()) {
          float mouse_x = ImGui::GetMousePos().x;
          float new_val = (mouse_x - slider_x) / slider_w;
          new_val = glm::clamp(new_val, 0.f, 1.f);
          if (new_val != info.zoom_normalized) {
            info.zoom_normalized = new_val;
            zoom_changed = true;
          }
        }

        /// advance cursor past the bar
        ImGui::SetCursorScreenPos({ bar_min.x, bar_max.y });
        ImGui::Dummy(ImVec2(0, 0));

        return zoom_changed;
      }

    }  // namespace asset_browser_w
  }  // namespace ui
}  // namespace other
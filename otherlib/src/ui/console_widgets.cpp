/**
 * \file ui/console_widgets.cpp
 **/
#include "ui/console_widgets.hpp"

#include <algorithm>
#include <format>
#include <span>

#include "renderer/ui/colors.hpp"
#include "renderer/ui/unicode.hpp"

#include "console_widgets.hpp"

namespace other {
  namespace ui {
    namespace console_w {
      namespace detail {

        int calculate_lines_in_text(const std::string& text) {
          int lines = 1;
          for (char c : text) {
            if (c == '\n') {
              lines++;
            }
          }
          return lines;
        }

      }  // namespace detail

      glm::vec4 color_for_level(log_level level) {
        using namespace colors::console;
        switch (level) {
          case log_level::COMMAND: return kCommand;
          case log_level::OUTPUT: return kOutput;
          case log_level::TRACE: return kTrace;
          case log_level::DEBUG: return kDebug;
          case log_level::INFO: return kInfo;
          case log_level::WARNING: return kWarning;
          case log_level::ERROR_ALERT: return kError;
          case log_level::FATAL: return kFatal;
          default: return kOutput;
        }
      }

      const char* prefix_for_level(log_level level) {
        switch (level) {
          case log_level::COMMAND: return "CMD";
          case log_level::OUTPUT: return "OUT";
          case log_level::TRACE: return "TRC";
          case log_level::DEBUG: return "DBG";
          case log_level::INFO: return "INF";
          case log_level::WARNING: return "WRN";
          case log_level::ERROR_ALERT: return "ERR";
          case log_level::FATAL: return "FTL";
          default: return "???";
        }
      }

      void draw_title_bar(const std::string_view title) {
        using namespace colors::console;

        ImDrawList* dl = ImGui::GetWindowDrawList();
        const ImVec2 cursor = ImGui::GetCursorScreenPos();
        const float avail_w = ImGui::GetContentRegionAvail().x;

        /// background
        ImVec2 bar_min = cursor;
        ImVec2 bar_max = { cursor.x + avail_w, cursor.y + kTitleBarHeight };
        dl->AddRectFilled(bar_min, bar_max, colors::to_im_col(kPromptBG), 0.f);

        /// bottom border
        dl->AddLine(
          { bar_min.x, bar_max.y },
          { bar_max.x, bar_max.y },
          colors::to_im_col(colors::console::kBorder), 1.f
        );

        /// status dot (accent warm orange)
        float dot_x = cursor.x + 14.f;
        float dot_y = cursor.y + kTitleBarHeight * 0.5f;
        dl->AddCircleFilled({ dot_x, dot_y }, 4.f, colors::to_im_col(kPromptSymbol));

        /// title text
        float text_x = dot_x + 12.f;
        float text_y = cursor.y + (kTitleBarHeight - ImGui::GetFontSize()) * 0.5f;
        dl->AddText({ text_x, text_y }, colors::to_im_col(kTimestamp), title.data(), title.data() + title.size());

        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + kTitleBarHeight);
      }

      filter_result draw_filter_bar(uint8_t& filter_mask, char* search_buf, uint32_t search_buf_size) {
        using namespace colors::console;
        filter_result result;
        result.changed = false;

        ImDrawList* dl = ImGui::GetWindowDrawList();
        const ImVec2 cursor = ImGui::GetCursorScreenPos();
        const float avail_w = ImGui::GetContentRegionAvail().x;

        /// background
        ImVec2 bar_min = cursor;
        ImVec2 bar_max = { cursor.x + avail_w, cursor.y + kFilterBarHeight };
        dl->AddRectFilled(bar_min, bar_max, colors::to_im_col(kPromptBG), 0.f);

        /// bottom separator
        dl->AddLine(
          { bar_min.x, bar_max.y },
          { bar_max.x, bar_max.y },
          colors::to_im_col(kBorder), 1.f
        );

        float btn_x = cursor.x + kPaddingX;
        float btn_y = cursor.y + 3.f;

        for (size_t i = 0; i < kNumFilterButtons; i++) {
          const auto& btn = kFilterButtons[i];

          uint8_t bit = static_cast<uint8_t>(btn.lvl);
          bool enabled = (filter_mask & bit) != 0;

          ImVec2 text_size = ImGui::CalcTextSize(btn.label);
          float btn_w = text_size.x + 12.f;
          float btn_h = kFilterBarHeight - 6.f;

          ImVec2 btn_min = { btn_x, btn_y };
          ImVec2 btn_max = { btn_x + btn_w, btn_y + btn_h };

          ImGui::SetCursorScreenPos(btn_min);
          std::string id = std::format("##filter_btn_{}{}", i, btn.label);
          if (ImGui::InvisibleButton(id.c_str(), { btn_w, btn_h })) {
            result.filter_changed[i] = true;
            if (enabled) {
              filter_mask &= ~bit;
              if (btn.lvl != console_w::log_level::COMMAND && btn.lvl != console_w::log_level::OUTPUT) {
                for (size_t j = i - 1; j > 1; j--) {
                  result.filter_changed[j] = true;
                  filter_mask &= ~static_cast<uint8_t>(kFilterButtons[j].lvl);
                }
              }
            }
            /// this condition ensures that COMMAND and OUTPUT levels are not toggled and also that i - 1 is valid
            else {
              filter_mask |= static_cast<uint8_t>(btn.lvl);
              for (size_t j = i + 1; j < kNumFilterButtons; j++) {
                result.filter_changed[j] = true;
                filter_mask |= static_cast<uint8_t>(kFilterButtons[j].lvl);
              }
            }
          }

          bool hovered = ImGui::IsItemHovered();
          glm::vec4 text_col = {};
          if (enabled) {
            text_col = color_for_level(btn.lvl);
          } else if (hovered) {
            text_col = kFilterHover;
          } else {
            text_col = kFilterInactive;
          }

          if (enabled) {
            dl->AddRectFilled(btn_min, btn_max, colors::to_im_col(glm::vec4(text_col.r, text_col.g, text_col.b, 0.12f)), 3.f);
          }

          float lx = btn_min.x + (btn_w - text_size.x) * 0.5f;
          float ly = btn_min.y + (btn_h - text_size.y) * 0.5f;
          dl->AddText({ lx, ly }, colors::to_im_col(text_col), btn.label);

          btn_x += btn_w + 4.f;
        }

        /// search input on the right side
        float search_w = 120.f;
        float search_x = cursor.x + avail_w - kPaddingX - search_w;

        ImGui::SetCursorScreenPos({ search_x, btn_y });
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6.f, 2.f));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.f);
        ImGui::PushStyleColor(ImGuiCol_FrameBg, colors::rgba_to_imvec4(kBG));
        ImGui::PushStyleColor(ImGuiCol_Border, colors::rgba_to_imvec4(kBorder));
        ImGui::PushStyleColor(ImGuiCol_Text, colors::rgba_to_imvec4(kOutput));
        ImGui::PushItemWidth(search_w);

        if (ImGui::InputTextWithHint("##console_search", "filter...", search_buf, search_buf_size)) {
          result.changed = true;
        }

        ImGui::PopItemWidth();
        ImGui::PopStyleColor(3);
        ImGui::PopStyleVar(2);

        ImGui::SetCursorScreenPos({ cursor.x, bar_max.y });

        result.changed = result.changed || std::ranges::any_of(std::span(result.filter_changed, kNumFilterButtons), [](bool v) { return v; });
        return result;
      }

      bool draw_log_line(const log_entry& entry, bool alt_row) {
        using namespace colors::console;

        ImDrawList* dl = ImGui::GetWindowDrawList();
        const ImVec2 cursor = ImGui::GetCursorScreenPos();
        const float avail_w = ImGui::GetContentRegionAvail().x;

        /// row background (alternating)
        int lines_in_text = detail::calculate_lines_in_text(entry.message);
        // float row_h = static_cast<float>(lines_in_text) * kLogLineHeight;
        float line_h = ImGui::GetTextLineHeight();
        float pad_y = (kLogLineHeight - line_h) * 0.5f;
        if (pad_y < 0.f) {
          pad_y = 0.f;
        }

        float row_h = std::max(kLogLineHeight, static_cast<float>(lines_in_text) * line_h + pad_y * 2.f);

        ImVec2 row_min = cursor;
        ImVec2 row_max = { cursor.x + avail_w, cursor.y + row_h };

        if (alt_row) {
          dl->AddRectFilled(row_min, row_max, colors::to_im_col(kBGAlt));
        }

        /// invisible button for hover / interaction
        ImGui::SetCursorScreenPos(cursor);
        std::string row_id = std::format("##log_{}", (uintptr_t)&entry);
        ImGui::InvisibleButton(row_id.c_str(), { avail_w, row_h });

        bool hovered = ImGui::IsItemHovered();
        if (hovered) {
          dl->AddRectFilled(row_min, row_max, IM_COL32(255, 255, 255, 8));
        }

        float text_y = cursor.y + pad_y;
        float x = cursor.x + kPaddingX;

        /// timestamp
        size_t timestamp_width = ImGui::CalcTextSize(entry.timestamp.c_str()).x;
        if (!entry.timestamp.empty()) {
          dl->AddText({ x, text_y }, colors::to_im_col(kTimestamp), entry.timestamp.c_str());
          x += timestamp_width + kPaddingX;
        }

        /// prompt symbol for command lines
        size_t prompt_width = ImGui::CalcTextSize(unicode::kPromptSymbol).x;
        if (entry.level == log_level::COMMAND) {
          const char* prompt = unicode::kPromptSymbol;
          dl->AddText({ x, text_y }, colors::to_im_col(kPromptSymbol), prompt);
          x += prompt_width + kPaddingX;
        }

        /// source tag (if present)
        size_t src_tag_width = ImGui::CalcTextSize(entry.source.c_str()).x;
        if (!entry.source.empty()) {
          std::string src_text = std::format("[{}] ", entry.source);
          dl->AddText({ x, text_y }, colors::to_im_col(kSource), src_text.c_str());
          x += src_tag_width + kPaddingX;
        }

        /// message text with inline span parsing
        /// Parses:  <TypeName>  →  kResultType color
        ///          followed by values like 0x4A2F or (0, 1.5, 0) → kResultValue
        glm::vec4 msg_color = color_for_level(entry.level);
        const std::string& msg = entry.message;

        size_t pos = 0;
        while (pos < msg.size()) {
          /// look for <type> spans
          size_t angle_open = msg.find('<', pos);
          size_t angle_close = (angle_open != std::string::npos) ? msg.find('>', angle_open) : std::string::npos;

          /// look for value patterns:  0x..., (...)
          size_t val_start = std::string::npos;
          size_t val_end = std::string::npos;

          /// find 0x hex values
          size_t hex_pos = msg.find("0x", pos);
          if (hex_pos != std::string::npos && hex_pos < msg.size()) {
            val_start = hex_pos;
            val_end = hex_pos + 2;
            while (val_end < msg.size() &&
                   ((msg[val_end] >= '0' && msg[val_end] <= '9') ||
                    (msg[val_end] >= 'A' && msg[val_end] <= 'F') ||
                    (msg[val_end] >= 'a' && msg[val_end] <= 'f'))) {
              val_end++;
            }
          }

          /// find (...) value tuples
          size_t paren_pos = msg.find('(', pos);
          if (paren_pos != std::string::npos) {
            size_t paren_close = msg.find(')', paren_pos);
            if (paren_close != std::string::npos) {
              /// use this if it comes before hex
              if (val_start == std::string::npos || paren_pos < val_start) {
                val_start = paren_pos;
                val_end = paren_close + 1;
              }
            }
          }

          /// determine which special span comes first
          size_t next_special = std::string::npos;
          bool is_type_span = false;

          if (angle_open != std::string::npos && angle_close != std::string::npos) {
            next_special = angle_open;
            is_type_span = true;
          }

          if (val_start != std::string::npos &&
              (next_special == std::string::npos || val_start < next_special)) {
            next_special = val_start;
            is_type_span = false;
          }

          if (next_special == std::string::npos || next_special >= msg.size()) {
            /// no more special spans — draw remaining text
            std::string_view remaining(msg.data() + pos, msg.size() - pos);
            dl->AddText({ x, text_y }, colors::to_im_col(msg_color), remaining.data(), remaining.data() + remaining.size());
            x += ImGui::CalcTextSize(remaining.data(), remaining.data() + remaining.size()).x;
            break;
          }

          /// draw plain text before the special span
          if (next_special > pos) {
            std::string_view before(msg.data() + pos, next_special - pos);
            dl->AddText({ x, text_y }, colors::to_im_col(msg_color), before.data(), before.data() + before.size());
            x += ImGui::CalcTextSize(before.data(), before.data() + before.size()).x;
          }

          /// draw the special span
          if (is_type_span) {
            std::string_view type_text(msg.data() + angle_open, angle_close - angle_open + 1);
            dl->AddText({ x, text_y }, colors::to_im_col(kResultType), type_text.data(), type_text.data() + type_text.size());
            x += ImGui::CalcTextSize(type_text.data(), type_text.data() + type_text.size()).x;
            pos = angle_close + 1;
          } else {
            std::string_view val_text(msg.data() + val_start, val_end - val_start);
            dl->AddText({ x, text_y }, colors::to_im_col(kResultValue), val_text.data(), val_text.data() + val_text.size());
            x += ImGui::CalcTextSize(val_text.data(), val_text.data() + val_text.size()).x;
            pos = val_end;
          }
        }

        /// advance cursor past the row
        ImGui::SetCursorScreenPos({ cursor.x, row_max.y });

        return hovered;
      }

      prompt_result draw_prompt_bar(char* input_buf, uint32_t buf_size, bool focus_requested) {
        using namespace colors::console;

        prompt_result result{};
        ImDrawList* dl = ImGui::GetWindowDrawList();
        const ImVec2 cursor = ImGui::GetCursorScreenPos();
        const float avail_w = ImGui::GetContentRegionAvail().x;

        /// background
        ImVec2 bar_min = cursor;
        ImVec2 bar_max = { cursor.x + avail_w, cursor.y + kPromptBarHeight };
        dl->AddRectFilled(bar_min, bar_max, colors::to_im_col(kPromptBG), 0.f);

        /// top border
        dl->AddLine(
          { bar_min.x, bar_min.y },
          { bar_max.x, bar_min.y },
          colors::to_im_col(kPromptBorder), 1.f
        );

        /// prompt symbol ❯
        float sym_x = cursor.x + kPaddingX;
        float text_y = cursor.y + (kPromptBarHeight - ImGui::GetFontSize()) * 0.5f;
        const char* prompt_sym = unicode::kPromptSymbol;  // ❯ UTF-8
        dl->AddText({ sym_x, text_y }, colors::to_im_col(kPromptSymbol), prompt_sym);

        /// input field
        float input_x = sym_x + kPromptSymbolWidth + 4.f;
        float input_w = avail_w - input_x + cursor.x - kPaddingX;

        ImGui::SetCursorScreenPos({ input_x, cursor.y + 4.f });
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4.f, 4.f));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.f);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.f);
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_Text, colors::rgba_to_imvec4(kPromptText));
        ImGui::PushItemWidth(input_w);

        if (focus_requested) {
          ImGui::SetKeyboardFocusHere();
        }

        ImGuiInputTextFlags flags = ImGuiInputTextFlags_EnterReturnsTrue;  // | ImGuiInputTextFlags_CallbackHistory | ImGuiInputTextFlags_CallbackCompletion;

        /// we use a simple InputText and check key states ourselves
        /// for more control over history/tab/escape
        if (ImGui::InputText("##console_input", input_buf, buf_size, flags)) {  //}, &detail::text_callback)) {
          result.submitted = true;
        }

        /// check key presses while input is active
        if (ImGui::IsItemActive()) {
          if (ImGui::IsKeyPressed(ImGuiKey_Tab)) {
            result.tab_pressed = true;
          }
          if (ImGui::IsKeyPressed(ImGuiKey_UpArrow)) {
            result.up_pressed = true;
          }
          if (ImGui::IsKeyPressed(ImGuiKey_DownArrow)) {
            result.down_pressed = true;
          }
          if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
            result.escape_pressed = true;
          }
        }

        ImGui::PopItemWidth();
        ImGui::PopStyleColor(2);
        ImGui::PopStyleVar(3);

        ImGui::SetCursorScreenPos({ cursor.x, bar_max.y });

        return result;
      }

      autocomplete_result draw_autocomplete_popup(const autocomplete_item* items, uint32_t item_count, int32_t current_index, const ImVec2& anchor_pos) {
        using namespace colors::console;

        autocomplete_result result{};
        result.selected_index = current_index;

        if (item_count == 0) return result;

        ImDrawList* dl = ImGui::GetForegroundDrawList();

        uint32_t visible_count = item_count;
        if (visible_count > static_cast<uint32_t>(kAutocompleteMaxVisible)) {
          visible_count = static_cast<uint32_t>(kAutocompleteMaxVisible);
        }

        float popup_h = static_cast<float>(visible_count) * kAutocompleteRowHeight + 4.f;
        float popup_w = 300.f;

        /// position above the anchor
        ImVec2 popup_min = { anchor_pos.x, anchor_pos.y - popup_h - 2.f };
        ImVec2 popup_max = { popup_min.x + popup_w, popup_min.y + popup_h };

        /// clamp to screen
        if (popup_min.y < 0.f) {
          /// show below instead
          popup_min.y = anchor_pos.y + kPromptBarHeight + 2.f;
          popup_max.y = popup_min.y + popup_h;
        }

        /// background
        dl->AddRectFilled(popup_min, popup_max, colors::to_im_col(kAutocompleteBG), 4.f);
        dl->AddRect(popup_min, popup_max, colors::to_im_col(kAutocompleteBorder), 4.f);

        /// draw items
        float y = popup_min.y + 2.f;
        for (uint32_t i = 0; i < visible_count; ++i) {
          /// handle scroll offset if more items than visible
          uint32_t item_idx = i;
          if (item_count > visible_count && current_index >= 0) {
            int32_t start = current_index - static_cast<int32_t>(visible_count) / 2;
            if (start < 0) start = 0;
            if (start + static_cast<int32_t>(visible_count) > static_cast<int32_t>(item_count)) {
              start = static_cast<int32_t>(item_count) - static_cast<int32_t>(visible_count);
            }
            item_idx = static_cast<uint32_t>(start) + i;
          }

          if (item_idx >= item_count) break;

          const auto& item = items[item_idx];
          ImVec2 row_min = { popup_min.x + 2.f, y };
          ImVec2 row_max = { popup_max.x - 2.f, y + kAutocompleteRowHeight };

          /// selection highlight
          bool selected = (static_cast<int32_t>(item_idx) == current_index);
          if (selected) {
            dl->AddRectFilled(row_min, row_max, colors::to_im_col(kAutocompleteSelected), 3.f);
          }

          float text_y = y + (kAutocompleteRowHeight - ImGui::GetFontSize()) * 0.5f;

          /// label — highlight matched portion
          float lx = popup_min.x + kAutocompletePaddingX;
          if (!item.match_highlight.empty()) {
            /// draw the matching prefix in accent color
            dl->AddText({ lx, text_y }, colors::to_im_col(kAutocompleteMatch), item.match_highlight.c_str());
            float match_w = ImGui::CalcTextSize(item.match_highlight.c_str()).x;

            /// draw the rest in normal color
            if (item.label.size() > item.match_highlight.size()) {
              std::string_view rest(item.label.data() + item.match_highlight.size(), item.label.size() - item.match_highlight.size());
              dl->AddText({ lx + match_w, text_y }, colors::to_im_col(kAutocompleteText), rest.data(), rest.data() + rest.size());
            }
          } else {
            dl->AddText({ lx, text_y }, colors::to_im_col(kAutocompleteText), item.label.c_str());
          }

          /// description on the right
          if (!item.description.empty()) {
            ImVec2 desc_size = ImGui::CalcTextSize(item.description.c_str());
            float dx = popup_max.x - kAutocompletePaddingX - desc_size.x;
            dl->AddText({ dx, text_y }, colors::to_im_col(kAutocompleteDesc), item.description.c_str());
          }

          y += kAutocompleteRowHeight;
        }

        return result;
      }

      void draw_text_span(const std::string_view text, const glm::vec4& color) {
        ImGui::PushStyleColor(ImGuiCol_Text, colors::rgba_to_imvec4(color));
        ImGui::TextUnformatted(text.data(), text.data() + text.size());
        ImGui::PopStyleColor();
        ImGui::SameLine(0.f, 0.f);
      }

      void draw_type_span(const std::string_view type_name) {
        draw_text_span(type_name, colors::console::kResultType);
      }

      void draw_value_span(const std::string_view value_text) {
        draw_text_span(value_text, colors::console::kResultValue);
      }

    }  // namespace console_w
  }  // namespace ui
}  // namespace other
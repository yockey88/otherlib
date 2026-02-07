/**
 * \file ui/console_widgets.cpp
 **/
#include "ui/console_widgets.hpp"

#include "core/defines.hpp"
#include "core/logger.hpp"

#include "ui/colors.hpp"
#include "ui/unicode.hpp"

namespace other {
  namespace ui {
    namespace console_w {

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

      namespace detail {

        int text_callback(ImGuiInputTextCallbackData* data) {
          OTHER_ASSERT(data != nullptr, "ImGuiInputTextCallbackData is null");
          // OTHER_ASSERT(data->UserData != nullptr, "UserData is null in text_callback, expected console_history_node*");
          // console_history_node* console_node = reinterpret_cast<console_history_node*>(data->UserData);
          // environment_console::history_move move = environment_console::HISTORY_MOVE_NONE;
          // switch (data->EventKey) {
          //   case ImGuiKey_UpArrow: move = environment_console::HISTORY_MOVE_BACK; break;
          //   case ImGuiKey_DownArrow: move = environment_console::HISTORY_MOVE_FORWARD; break;
          //   default:
          //     break;
          // }

          // if (move != environment_console::HISTORY_MOVE_NONE) {
          //   CORE_LOG_DEBUG("text_callback: EventKey = {}, move = {}", data->EventKey, move);
          //   environment_console::move_history_cursor(move);
          //   char* input_buffer = environment_console::get_input_buffer();
          //   OTHER_ASSERT(input_buffer != nullptr, "Input buffer is null in text_callback");

          //   const auto& history = environment_console::get_console_history();
          //   size_t cursor = environment_console::get_cursor_position();
          //   if (cursor < history.size()) {
          //     std::strncpy(input_buffer, history[cursor].input_text.c_str(), environment_console::kInputBufferSize - 1);
          //   } else {
          //     environment_console::clear_input_buffer();
          //   }
          // }

          return 0;
        }

      }  // namespace detail

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

      bool draw_filter_bar(uint8_t& filter_mask, char* search_buf, uint32_t search_buf_size) {
        using namespace colors::console;
        bool changed = false;

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

        /// level toggle buttons: trace, debug, info, warning, error
        struct level_btn {
          const char* label;
          log_level lvl;
        };

        static const level_btn buttons[] = {
          { "TRC", log_level::TRACE },
          { "DBG", log_level::DEBUG },
          { "INF", log_level::INFO },
          { "WRN", log_level::WARNING },
          { "ERR", log_level::ERROR_ALERT },
        };

        for (const auto& btn : buttons) {
          uint8_t bit = 1u << static_cast<uint8_t>(btn.lvl);
          bool enabled = (filter_mask & bit) != 0;

          ImVec2 text_size = ImGui::CalcTextSize(btn.label);
          float btn_w = text_size.x + 12.f;
          float btn_h = kFilterBarHeight - 6.f;

          ImVec2 btn_min = { btn_x, btn_y };
          ImVec2 btn_max = { btn_x + btn_w, btn_y + btn_h };

          ImGui::SetCursorScreenPos(btn_min);
          std::string id = std::format("##filter_{}", btn.label);
          if (ImGui::InvisibleButton(id.c_str(), { btn_w, btn_h })) {
            filter_mask ^= bit;
            changed = true;
          }

          bool hovered = ImGui::IsItemHovered();
          glm::vec4 text_col = enabled ? color_for_level(btn.lvl) :
            hovered                    ? kFilterHover :
                                         kFilterInactive;

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
          changed = true;
        }

        ImGui::PopItemWidth();
        ImGui::PopStyleColor(3);
        ImGui::PopStyleVar(2);

        ImGui::SetCursorScreenPos({ cursor.x, bar_max.y });

        return changed;
      }

      bool draw_log_line(const log_entry& entry, bool alt_row) {
        using namespace colors::console;

        ImDrawList* dl = ImGui::GetWindowDrawList();
        const ImVec2 cursor = ImGui::GetCursorScreenPos();
        const float avail_w = ImGui::GetContentRegionAvail().x;

        /// row background (alternating)
        ImVec2 row_min = cursor;
        ImVec2 row_max = { cursor.x + avail_w, cursor.y + kLogLineHeight };

        if (alt_row) {
          dl->AddRectFilled(row_min, row_max, colors::to_im_col(kBGAlt));
        }

        /// invisible button for hover / interaction
        ImGui::SetCursorScreenPos(cursor);
        std::string row_id = std::format("##log_{}", (uintptr_t)&entry);
        ImGui::InvisibleButton(row_id.c_str(), { avail_w, kLogLineHeight });
        bool hovered = ImGui::IsItemHovered();

        if (hovered) {
          dl->AddRectFilled(row_min, row_max, IM_COL32(255, 255, 255, 8));
        }

        float text_y = cursor.y + (kLogLineHeight - ImGui::GetFontSize()) * 0.5f;
        float x = cursor.x + kPaddingX;

        /// timestamp
        if (!entry.timestamp.empty()) {
          dl->AddText({ x, text_y }, colors::to_im_col(kTimestamp), entry.timestamp.c_str());
          x += kTimestampWidth;
        }

        /// prompt symbol for command lines
        if (entry.level == log_level::COMMAND) {
          const char* prompt = unicode::kPromptSymbol;
          dl->AddText({ x, text_y }, colors::to_im_col(kPromptSymbol), prompt);
          x += kPromptSymbolWidth;
        }

        /// source tag (if present)
        if (!entry.source.empty()) {
          std::string src_text = std::format("[{}] ", entry.source);
          dl->AddText({ x, text_y }, colors::to_im_col(kSource), src_text.c_str());
          x += ImGui::CalcTextSize(src_text.c_str()).x;
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

      autocomplete_result draw_autocomplete_popup(
        const autocomplete_item* items, uint32_t item_count,
        int32_t current_index, const ImVec2& anchor_pos
      ) {
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
        ImVec2 display_size = ImGui::GetIO().DisplaySize;
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
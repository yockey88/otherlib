/**
 * \file ui/console_widgets.cpp
 **/
#include "ui/console_widgets.hpp"

#include <algorithm>
#include <cctype>
#include <format>
#include <span>
#include <string>

#include "core/logger.hpp"

#include "renderer/ui/colors.hpp"
#include "renderer/ui/unicode.hpp"

#include "driver/driver.hpp"

#include "console_widgets.hpp"

namespace other {
  namespace ui {
    namespace console_w {
      namespace detail {

        bool contains_newline(const std::string& text) {
          return text.find('\n') != std::string::npos || text.find('\r') != std::string::npos;
        }

        std::string normalize_newlines(const std::string& text) {
          std::string out;
          out.reserve(text.size());

          for (size_t i = 0; i < text.size(); ++i) {
            char c = text[i];
            if (c == '\r') {
              if (i + 1 < text.size() && text[i + 1] == '\n') {
                ++i;
              }
              out.push_back('\n');
              continue;
            }
            out.push_back(c);
          }

          return out;
        }

        bool is_type_span_candidate(const std::string_view span) {
          if (span.size() < 3 || span.front() != '<' || span.back() != '>') {
            return false;
          }

          const std::string_view inner = span.substr(1, span.size() - 2);
          if (inner.empty()) {
            return false;
          }

          for (char c : inner) {
            const unsigned char uc = static_cast<unsigned char>(c);
            if (std::isalnum(uc) != 0 || c == '_' || c == ':') {
              continue;
            }
            return false;
          }

          return true;
        }

        bool is_value_tuple_candidate(const std::string_view span) {
          if (span.size() < 3 || span.front() != '(' || span.back() != ')') {
            return false;
          }

          const std::string_view inner = span.substr(1, span.size() - 2);
          for (char c : inner) {
            if (std::isdigit(static_cast<unsigned char>(c)) != 0) {
              return true;
            }
          }
          return false;
        }

        int calculate_lines_in_text(const std::string& text) {
          int lines = 1;
          for (char c : text) {
            if (c == '\n') {
              lines++;
            }
          }
          return lines;
        }

        std::string get_name_of_mode(driver::mode mode) {
          return "OTHER";
          // switch (mode) {
          //   // case driver::mode::STOPPED: return "STOPPED";
          //   // case driver::mode::PLAYING: return "PLAYING";
          //   default: return "UNKNOWN";
          // }
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
        // dl->AddText({ x, text_y }, colors::to_im_col(msg_color), msg.c_str());

        size_t pos = 0;
        while (pos < msg.size()) {
          enum type {
            PLAIN,
            TYPEWORD,
            VALUE,
          };
          type chunk_type = type::PLAIN;

          if (msg[pos] == '\n' || msg[pos] == '\r') {
            pos++;
            text_y += line_h;
            x = cursor.x + kPaddingX;
            continue;
          }

          std::span remaining(msg.data() + pos, msg.size() - pos);

          std::string next_chunk = {};
          if (std::isalpha(msg[pos]) || std::isdigit(msg[pos])) {
            next_chunk = remaining |
              std::views::take_while([&](char c) { return std::isalpha(c) || std::isdigit(c); }) |
              std::ranges::to<std::string>();
          } else if (msg[pos] == '<' || msg[pos] == '(') {
            next_chunk = remaining |
              std::views::take_while([&](char c) { return c != '\n' && c != '\r' && c != '>' && c != ')'; }) |
              std::ranges::to<std::string>();
            if (msg[pos + next_chunk.size()] == '>' || msg[pos + next_chunk.size()] == ')') {
              next_chunk += msg[pos + next_chunk.size()];
            }

            if (detail::is_type_span_candidate(next_chunk)) {
              chunk_type = type::TYPEWORD;
            } else if (detail::is_value_tuple_candidate(next_chunk)) {
              chunk_type = type::VALUE;
            }
          } else {
            next_chunk = std::string(1, msg[pos]);
          }
          pos += next_chunk.size();

          auto color = colors::to_im_col([chunk_type, msg_color]() {
            switch (chunk_type) {
              case type::TYPEWORD: return kResultType;
              case type::VALUE: return kResultValue;
              default: return msg_color;
            }
          }());

          dl->AddText({ x, text_y }, color, next_chunk.data(), next_chunk.data() + next_chunk.size());
          x += ImGui::CalcTextSize(next_chunk.data(), next_chunk.data() + next_chunk.size()).x;
        }

        /// advance cursor past the row
        ImGui::SetCursorScreenPos({ cursor.x, row_max.y });

        return hovered;
      }

      prompt_result draw_prompt_bar(driver* driver_ptr, char* input_buf, uint32_t buf_size, bool focus_requested) {
        OTHER_ASSERT(driver_ptr != nullptr, "Driver pointer is null");
        using namespace colors::console;

        prompt_result result{};
        ImDrawList* dl = ImGui::GetWindowDrawList();
        const ImVec2 cursor = ImGui::GetCursorScreenPos();
        const float avail_w = ImGui::GetContentRegionAvail().x;

        // ------------------------------------
        // |  [mode-name]>                    |
        // ------------------------------------

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

        // mode name
        float text_y = cursor.y + (kPromptBarHeight - ImGui::GetFontSize()) * 0.5f;
        std::string mode_text = "[ " + detail::get_name_of_mode(driver_ptr->get_current_mode()) + " ]";
        dl->AddText({ cursor.x, text_y }, colors::to_im_col(kPromptText), mode_text.c_str());

        /// prompt symbol ❯
        float sym_x = cursor.x + ImGui::CalcTextSize(mode_text.c_str()).x;
        const char* prompt_sym = unicode::kPromptSymbol;  // ❯ UTF-8
        dl->AddText({ sym_x, text_y }, colors::to_im_col(kPromptSymbol), prompt_sym);

        /// input field
        float input_x = sym_x + (kPaddingX / 2.f) + 4.f;
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
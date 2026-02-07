/**
 * \file ui/console_widgets.hpp
 * */
#ifndef OTHERLIB_UI_CONSOLE_WIDGETS_HPP
#define OTHERLIB_UI_CONSOLE_WIDGETS_HPP

#include <cstdint>
#include <string>

#include <glm/glm.hpp>
#include <imgui/imgui.h>

namespace other {

  class driver;
  class scene;
  struct scene_object;

  namespace ui {
    namespace console_w {

      constexpr float kTitleBarHeight = 26.f;
      constexpr float kLogLineHeight = 20.f;
      constexpr float kPromptBarHeight = 30.f;
      constexpr float kFilterBarHeight = 24.f;
      constexpr float kTimestampWidth = 64.f;
      constexpr float kPaddingX = 16.f;
      constexpr float kPaddingY = 8.f;
      constexpr float kPromptSymbolWidth = 18.f;
      constexpr float kAutocompleteRowHeight = 22.f;
      constexpr float kAutocompleteMaxVisible = 8;
      constexpr float kAutocompletePaddingX = 10.f;

      enum class log_level : uint8_t {
        COMMAND = 0,  ///< user-typed command echo
        OUTPUT,       ///< standard command output
        TRACE,        ///< verbose / low-priority
        DEBUG,        ///< debug-level
        INFO,         ///< informational
        WARNING,      ///< warning
        ERROR_ALERT,  ///< error
        FATAL,        ///< fatal / unrecoverable
      };

      struct log_entry {
        std::string timestamp;  ///< formatted time string "[HH:MM:SS]"
        std::string message;    ///< the log text
        log_level level = log_level::OUTPUT;
        std::string source;  ///< optional source tag (e.g. "Renderer")
      };

      struct prompt_result {
        bool submitted = false;       ///< enter was pressed
        bool tab_pressed = false;     ///< tab for autocomplete
        bool up_pressed = false;      ///< up arrow for history
        bool down_pressed = false;    ///< down arrow for history
        bool escape_pressed = false;  ///< escape to dismiss autocomplete
      };

      struct autocomplete_item {
        std::string label;            ///< command name
        std::string description;      ///< short description
        std::string match_highlight;  ///< portion that matched (for highlight)
      };

      struct autocomplete_result {
        int32_t selected_index = -1;  ///< -1 = nothing selected
        bool confirmed = false;       ///< user pressed enter/tab on selection
        bool dismissed = false;       ///< user pressed escape
      };

      glm::vec4 color_for_level(log_level level);
      const char* prefix_for_level(log_level level);

      void draw_title_bar(const std::string_view title);
      bool draw_filter_bar(uint8_t& filter_mask, char* search_buf, uint32_t search_buf_size);
      bool draw_log_line(const log_entry& entry, bool alt_row);
      prompt_result draw_prompt_bar(char* input_buf, uint32_t buf_size, bool focus_requested);
      autocomplete_result draw_autocomplete_popup(const autocomplete_item* items, uint32_t item_count, int32_t current_index, const ImVec2& anchor_pos);

      void draw_text_span(const std::string_view text, const glm::vec4& color);
      void draw_type_span(const std::string_view type_name);
      void draw_value_span(const std::string_view value_text);

    }  // namespace console_w
  }  // namespace ui
}  // namespace other

#endif  // OTHERLIB_UI_CONSOLE_WIDGETS_HPP
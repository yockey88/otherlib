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
        COMMAND = 1 << 0,
        OUTPUT = 1 << 1,
        TRACE = 1 << 2,
        DEBUG = 1 << 3,
        INFO = 1 << 4,
        WARNING = 1 << 5,
        ERROR_ALERT = 1 << 6,
        FATAL = 1 << 7,
      };

      struct level_btn {
        const char* label;
        log_level lvl;
      };

      constexpr static inline level_btn kFilterButtons[] = {
        { "CMD", log_level::COMMAND },
        { "OUT", log_level::OUTPUT },
        { "TRC", log_level::TRACE },
        { "DBG", log_level::DEBUG },
        { "INF", log_level::INFO },
        { "WRN", log_level::WARNING },
        { "ERR", log_level::ERROR_ALERT },
      };
      constexpr static inline size_t kNumFilterButtons = sizeof(kFilterButtons) / sizeof(kFilterButtons[0]);

      struct filter_result {
        bool changed = false;
        bool filter_changed[kNumFilterButtons] = {};
      };

      struct log_entry {
        std::string timestamp;
        std::string message;
        log_level level = log_level::OUTPUT;
        std::string source;
      };

      struct prompt_result {
        bool submitted = false;
        bool tab_pressed = false;
        bool up_pressed = false;
        bool down_pressed = false;
        bool escape_pressed = false;
      };

      struct autocomplete_item {
        std::string label;
        std::string description;
        std::string match_highlight;
      };

      struct autocomplete_result {
        int32_t selected_index = -1;
        bool confirmed = false;
        bool dismissed = false;
      };

      glm::vec4 color_for_level(log_level level);
      const char* prefix_for_level(log_level level);

      void draw_title_bar(const std::string_view title);
      filter_result draw_filter_bar(uint8_t& filter_mask, char* search_buf, uint32_t search_buf_size);
      bool draw_log_line(const log_entry& entry, bool alt_row);
      prompt_result draw_prompt_bar(driver* driver_ptr, char* input_buf, uint32_t buf_size, bool focus_requested);
      autocomplete_result draw_autocomplete_popup(const autocomplete_item* items, uint32_t item_count, int32_t current_index, const ImVec2& anchor_pos);

      void draw_text_span(const std::string_view text, const glm::vec4& color);
      void draw_type_span(const std::string_view type_name);
      void draw_value_span(const std::string_view value_text);

    }  // namespace console_w
  }  // namespace ui
}  // namespace other

#endif  // OTHERLIB_UI_CONSOLE_WIDGETS_HPP
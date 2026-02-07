/**
 * \file ui/console_history_node.cpp
 **/
#include "ui/console_history_node.hpp"

#include <chrono>
#include <format>

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

#include "core/defines.hpp"

#include "tools/environment_console.hpp"
#include "ui/colors.hpp"

#include "console_widgets.hpp"

namespace other {
  namespace ui {

    console_history_node::console_history_node(ui_window* window, driver* drvr)
        : ui_node(window, "Console Output"), driver_ptr(drvr) {
      /// listen for log events from the engine
      events().add_listener("console.log", [this](const value& data) {
        /// expect a table-like value with "message", "level", "source" fields
        /// fall back to treating the whole value as a string
        if (data.type() == value_type::STRING) {
          push_log(static_cast<std::string>(data), console_w::log_level::OUTPUT);
          return;
        }
        CORE_LOG_ERROR("Structured log entry from engine systems unimplemented");

        // /// structured log entry from engine systems
        // std::string message = "";
        // console_w::log_level level = console_w::log_level::OUTPUT;

        // std::string source;
        // if (data.has("message")) {
        //   message = static_cast<std::string>(data["message"]);
        // }
        // if (data.has("level")) {
        //   int lvl_int = static_cast<int>(data["level"]);
        //   if (lvl_int >= 0 && lvl_int <= static_cast<int>(console_w::log_level::fatal)) {
        //     level = static_cast<console_w::log_level>(lvl_int);
        //   }
        // }
        // if (data.has("source")) {
        //   source = static_cast<std::string>(data["source"]);
        // }

        // push_log(message, level, source);
      });

      /// listen for console command output events
      events().add_listener("console.output", [this](const value& data) {
        if (data.type() == value_type::STRING) {
          push_log(static_cast<std::string>(data), console_w::log_level::OUTPUT);
        }
      });

      /// listen for console command echo (shows the command the user typed)
      events().add_listener("console.command-echo", [this](const value& data) {
        if (data.type() == value_type::STRING) {
          std::string command_text = data;
          push_command(command_text);
        }
      });

      events().add_listener("console.clear", [this](const value&) { clear(); });
    }

    void console_history_node::push_log(console_w::log_entry entry) {
      if (entry.timestamp.empty()) {
        entry.timestamp = make_timestamp();
      }

      CORE_LOG_DEBUG("Pushed message :\n{}", entry.message);
      entries.push_back(std::move(entry));

      /// cap buffer size
      while (entries.size() > kMaxEntries) {
        entries.pop_front();
      }

      /// trigger auto-scroll
      if (auto_scroll) {
        scroll_to_bottom = true;
      }
    }

    void console_history_node::push_log(const std::string& message, console_w::log_level level, const std::string& source) {
      console_w::log_entry entry;
      entry.timestamp = make_timestamp();
      entry.message = message;
      entry.level = level;
      entry.source = source;
      push_log(std::move(entry));
    }

    void console_history_node::push_command(const std::string& command_text) {
      console_w::log_entry entry;
      entry.timestamp = make_timestamp();
      entry.message = command_text;
      entry.level = console_w::log_level::COMMAND;
      push_log(std::move(entry));
    }

    void console_history_node::clear() {
      entries.clear();
    }

    bool console_history_node::passes_filter(const console_w::log_entry& entry) const {
      /// always show commands and output
      if (entry.level == console_w::log_level::COMMAND ||
          entry.level == console_w::log_level::OUTPUT) {
        return search_lower.empty() ||
          entry.message.find(search_lower) != std::string::npos;
      }

      /// check level filter
      uint8_t bit = 1u << static_cast<uint8_t>(entry.level);
      if ((filter_mask & bit) == 0) {
        return false;
      }

      /// check text search
      if (!search_lower.empty()) {
        std::string msg_lower = entry.message;
        std::transform(msg_lower.begin(), msg_lower.end(), msg_lower.begin(), [](unsigned char c) { return std::tolower(c); });
        if (msg_lower.find(search_lower) == std::string::npos) {
          return false;
        }
      }

      return true;
    }

    std::string console_history_node::make_timestamp() {
      auto now = std::chrono::system_clock::now();
      auto time = std::chrono::system_clock::to_time_t(now);
      std::tm tm_buf{};

#ifdef _WIN32
      localtime_s(&tm_buf, &time);
#else
      localtime_r(&time, &tm_buf);
#endif

      return std::format("[{:02}:{:02}:{:02}]", tm_buf.tm_hour, tm_buf.tm_min, tm_buf.tm_sec);
    }

    void console_history_node::on_render_node_body() {
      namespace cw = console_w;
      using namespace colors;

      ImGui::PushStyleColor(ImGuiCol_ChildBg, rgba_to_imvec4(console::kBG));

      auto history_size = ImGui::GetContentRegionAvail().y - cw::kPromptBarHeight;

      if (!ImGui::BeginChild("##console-history", ImVec2(0, history_size), ImGuiChildFlags_None)) {
        ImGui::EndChild();
        ImGui::PopStyleColor();
        return;
      }

      /// title bar
      cw::draw_title_bar("CONSOLE \xe2\x80\x94 other engine");

      /// filter bar
      if (cw::draw_filter_bar(filter_mask, search_buf, sizeof(search_buf))) {
        /// update lowercase search cache
        search_lower = search_buf;
        std::transform(search_lower.begin(), search_lower.end(), search_lower.begin(), [](unsigned char c) { return std::tolower(c); });
      }

      /// scrollable log area
      float log_area_h = ImGui::GetContentRegionAvail().y;

      ImGui::PushStyleColor(ImGuiCol_ChildBg, rgba_to_imvec4(console::kBG));
      ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.f, cw::kPaddingY));

      if (ImGui::BeginChild("##log-scroll", ImVec2(0, log_area_h), ImGuiChildFlags_None)) {
        /// draw visible entries
        uint32_t visible_idx = 0;
        for (const auto& entry : entries) {
          if (!passes_filter(entry)) {
            continue;
          }

          bool alt = (visible_idx % 2) == 1;
          cw::draw_log_line(entry, alt);
          visible_idx++;
        }

        /// empty state
        if (visible_idx == 0 && entries.empty()) {
          float cx = ImGui::GetContentRegionAvail().x * 0.5f;
          float cy = ImGui::GetContentRegionAvail().y * 0.3f;
          ImGui::SetCursorPos({ cx - 60.f, cy });
          ImGui::PushStyleColor(ImGuiCol_Text, rgba_to_imvec4(console::kHint));
          ImGui::TextUnformatted("No console output yet.");
          ImGui::PopStyleColor();
        }

        /// auto-scroll
        if (scroll_to_bottom) {
          ImGui::SetScrollHereY(1.0f);
          scroll_to_bottom = false;
        }

        /// detect if user scrolled away from bottom
        if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY() - 4.f) {
          auto_scroll = true;
        } else if (ImGui::IsMouseDragging(0) || ImGui::GetIO().MouseWheel != 0.f) {
          auto_scroll = false;
        }
      }
      ImGui::EndChild();

      ImGui::PopStyleVar();
      ImGui::PopStyleColor();

      ImGui::EndChild();
      ImGui::PopStyleColor();
    }

  }  // namespace ui
}  // namespace other
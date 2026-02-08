/**
 * \file ui/console_history_node.cpp
 **/
#include "ui/console_history_node.hpp"

#include <chrono>
#include <ranges>

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

#include "core/defines.hpp"

#include "renderer/ui/colors.hpp"
#include "renderer/ui/unicode.hpp"

#include "console_widgets.hpp"

namespace other {
  namespace ui {
    namespace {

      void rtrim_trailing_newlines(std::string& text) {
        auto drop_and_flip = std::views::drop_while([](char c) { return c == '\n' || c == '\r' || std::isspace(c); }) | std::views::reverse;
        auto trim = drop_and_flip | drop_and_flip;
        text = text | trim | std::ranges::to<std::string>();
      }

    }  // namespace

    console_history_node::console_history_node(ui_window* window, driver* drvr)
        : ui_node(window, "Console Output"), driver_ptr(drvr) {
      /// listen for console command output events
      events().add_listener("console.trace", [this](const value& data) {
        if (data.type() == value_type::STRING) {
          handle_console_message(data, console_w::log_level::TRACE);
        }
      });
      events().add_listener("console.debug", [this](const value& data) {
        if (data.type() == value_type::STRING) {
          handle_console_message(data, console_w::log_level::DEBUG);
        }
      });
      events().add_listener("console.info", [this](const value& data) {
        if (data.type() == value_type::STRING) {
          handle_console_message(data, console_w::log_level::INFO);
        }
      });
      events().add_listener("console.warn", [this](const value& data) {
        if (data.type() == value_type::STRING) {
          handle_console_message(data, console_w::log_level::WARNING);
        }
      });
      events().add_listener("console.error", [this](const value& data) {
        if (data.type() == value_type::STRING) {
          handle_console_message(data, console_w::log_level::ERROR_ALERT);
        }
      });
      events().add_listener("console.critical", [this](const value& data) {
        if (data.type() == value_type::STRING) {
          handle_console_message(data, console_w::log_level::ERROR_ALERT);
        }
      });
      events().add_listener("console.output", [this](const value& data) {
        if (data.type() == value_type::STRING) {
          handle_console_message(data, console_w::log_level::OUTPUT);
        }
      });
      events().add_listener("console.command", [this](const value& data) {
        if (data.type() == value_type::STRING) {
          handle_console_message(data, console_w::log_level::COMMAND);
        }
      });

      events().add_listener("console.clear", [this](const value&) { clear(); });

      /// start with no TRACE and DEBUG levels enabled
      filter_mask &= ~static_cast<uint8_t>(console_w::log_level::TRACE);
      filter_mask &= ~static_cast<uint8_t>(console_w::log_level::DEBUG);
    }

    void console_history_node::push_log(console_w::log_entry entry) {
      if (entry.timestamp.empty()) {
        entry.timestamp = make_timestamp();
      }

      rtrim_trailing_newlines(entry.message);

      /// we also send it through the environment console
      environment_console::submit_console_text(entry.message, console_msg_type(entry.level));
      entries.push_back(std::move(entry));
      while (entries.size() > kMaxEntries) {
        entries.pop_front();
      }

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

    void console_history_node::clear() {
      entries.clear();
    }

    bool console_history_node::passes_filter(const console_w::log_entry& entry) const {
      /// check level filter
      uint8_t bit = static_cast<uint8_t>(entry.level);
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

    void console_history_node::handle_console_message(const value& msg, console_w::log_level level) {
      if (msg.type() == value_type::STRING) {
        std::string message_text = msg;
        push_log(message_text, level);
      }
    }

    console_message_type console_history_node::console_msg_type(console_w::log_level level) const {
      switch (level) {
        case console_w::log_level::COMMAND: return console_message_type::CONSOLE_MESSAGE_COMMAND;
        case console_w::log_level::TRACE: return console_message_type::CONSOLE_MESSAGE_TRACE;
        case console_w::log_level::DEBUG: return console_message_type::CONSOLE_MESSAGE_DEBUG;
        case console_w::log_level::INFO: return console_message_type::CONSOLE_MESSAGE_INFO;
        case console_w::log_level::WARNING: return console_message_type::CONSOLE_MESSAGE_WARN;

        case console_w::log_level::ERROR_ALERT:
        case console_w::log_level::FATAL:
          return console_message_type::CONSOLE_MESSAGE_ERROR;

        case console_w::log_level::OUTPUT:
        default:
          return console_message_type::CONSOLE_MESSAGE_MESSAGE;
      }
    }

    void console_history_node::on_render_node_body() {
      namespace cw = console_w;
      using namespace colors;

      ImGui::PushStyleColor(ImGuiCol_ChildBg, rgba_to_imvec4(console::kBG));

      /// Use negative size to reserve space at the bottom for the prompt bar.
      /// Computing a positive height can become negative in small layouts and
      /// would then be interpreted by ImGui as "avail - abs(size)", causing overlap.
      if (!ImGui::BeginChild("##console-history", ImVec2(0, -cw::kPromptBarHeight + 8.f), ImGuiChildFlags_None)) {
        ImGui::EndChild();
        ImGui::PopStyleColor();
        return;
      }

      /// title bar
      /// \todo get unicode symbol from engine mode
      const std::string console_title = std::format("CONSOLE {} other engine", unicode::kEmDash);
      cw::draw_title_bar(console_title);

      /// filter bar
      cw::filter_result filter_result = cw::draw_filter_bar(filter_mask, search_buf, sizeof(search_buf));
      if (filter_result.changed) {
        /// update lowercase search cache
        search_lower = search_buf;
        std::transform(search_lower.begin(), search_lower.end(), search_lower.begin(), [](unsigned char c) { return std::tolower(c); });
      }

      /// scrollable log area
      ImGui::PushStyleColor(ImGuiCol_ChildBg, rgba_to_imvec4(console::kBG));
      ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.f, cw::kPaddingY));

      if (ImGui::BeginChild("##log-scroll")) {
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
          auto_scroll = true;
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
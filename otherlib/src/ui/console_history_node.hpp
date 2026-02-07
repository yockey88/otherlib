/**
 * \file ui/console_history_node.hpp
 **/
#ifndef OTHERLIB_UI_CONSOLE_HISTORY_NODE_HPP
#define OTHERLIB_UI_CONSOLE_HISTORY_NODE_HPP

#include "renderer/ui/ui_node.hpp"

#include "ui/console_widgets.hpp"

namespace other {

  class driver;

  namespace ui {

    class console_history_node : public ui_node {
     public:
      console_history_node(ui_window* parent, driver* drvr);
      virtual ~console_history_node() = default;

      void push_log(console_w::log_entry entry);
      void push_log(const std::string& message, console_w::log_level level, const std::string& source = "");
      void push_command(const std::string& command_text);
      void clear();

      size_t entry_count() const { return entries.size(); }

     private:
      driver* driver_ptr = nullptr;

      static constexpr size_t kMaxEntries = 4096;
      std::deque<console_w::log_entry> entries;

      bool auto_scroll = true;        ///< stick to bottom
      bool scroll_to_bottom = false;  ///< force scroll this frame

      uint8_t filter_mask = 0xFF;  ///< all levels enabled by default
      char search_buf[256] = {};
      std::string search_lower;

      bool passes_filter(const console_w::log_entry& entry) const;

      void on_render_node_body() override;
      static std::string make_timestamp();
    };

  }  // namespace ui

}  // namespace other

#endif  // OTHERLIB_UI_CONSOLE_HISTORY_NODE_HPP
/**
 * \file terminal/terminal.hpp
 **/
#ifndef OTHER_TERMINAL_TERMINAL_HPP
#define OTHER_TERMINAL_TERMINAL_HPP

#include <string>

#include <SDL3/SDL.h>
#include <glm/glm.hpp>

#include "core/command_line.hpp"
#include "core/config_table.hpp"
#include "core/defines.hpp"
#include "thread/message.hpp"

#include "terminal/terminal_thread.hpp"

// #include "event/key_events.hpp"

namespace other {

  enum terminal_filter : uint16_t {
    NO_FILTER = 0,

    ERROR_FILTER = bit(0),
    WARNING_FILTER = bit(1),
    INFO_FILTER = bit(2),
    DEBUG_FILTER = bit(3),
    TRACE_FILTER = bit(4),

    BAD_FILTER = ERROR_FILTER | WARNING_FILTER,
    GOOD_FILTER = INFO_FILTER | DEBUG_FILTER | TRACE_FILTER,
    TERMINAL_FILTER_ALL = ERROR_FILTER | WARNING_FILTER | INFO_FILTER | DEBUG_FILTER | TRACE_FILTER,

    NUM_TERMINAL_FILTERS,
    INVALID_TERMINAL_FILTER = NUM_TERMINAL_FILTERS
  };

  struct terminal_message {
    terminal_filter filters;
    std::string message;
  };

  class terminal {
   public:
    terminal();
    ~terminal() = default;

    void run(const command_line& cmdline, const config_table& config);
    void on_key_down(SDL_Event* event);

   private:
    void initialize(const command_line& cmdline, const config_table& config);
    void update();
    void draw();
    void shutdown();

    void push_message(const terminal_message& message, bool save = true);
    void push_command(const terminal_message& command);

    glm::vec4 get_color_for_filter(terminal_filter filter) const;

    SDL_WindowID term_window_id = 0;
    bool is_running = true;

    static constexpr size_t kInputBufferSize = 1024;
    std::array<char, kInputBufferSize> input_buffer;

    opt<uint32_t> history_cursor = std::nullopt;
    std::vector<terminal_message> terminal_history;
    std::vector<terminal_message> stored_history;
    std::queue<terminal_message> message_buffer;

    command_parser cmd_parser;
    command_compiler compiler;

    void stop();

    void handle_input();
    void handle_received_thread_message(const message& msg);
    void handle_control_message(const message& msg);
  };

}  // namespace other

#endif  // OTHER_TERMINAL_TERMINAL_HPP
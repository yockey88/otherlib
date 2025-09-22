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

#include "renderer/render_pipeline.hpp"

#include "scene/scene.hpp"

#include "driver/driver.hpp"
#include "terminal/terminal-thread.hpp"

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

  class OTHER_CLASS terminal : public driver {
   public:
    terminal(const config_table& config);
    virtual ~terminal() = default;

    void on_initialize() override;
    void run() override;
    void on_shutdown() override;

    command_parser cmd_parser;
    opt<uint32_t> history_cursor = std::nullopt;

    static constexpr size_t kInputBufferSize = 1024;
    std::array<char, kInputBufferSize> input_buffer;
    std::vector<terminal_message> terminal_history;

    void on_event(SDL_Event* event) override;

    void update();
    void draw();

    void push_message(const terminal_message& message, bool save = true);
    void push_command(const terminal_message& command);

   private:
    glm::vec4 get_color_for_filter(terminal_filter filter) const;

    scene active_scene;
    scope<renderer> renderer = nullptr;

    scope<terminal_thread> control_thread = nullptr;

    bool is_running = false;

    std::vector<terminal_message> stored_history;
    std::queue<terminal_message> message_buffer;

    command_compiler compiler;

    // void stop();

    void handle_input();
    void handle_received_thread_message(const message& msg);
    void handle_control_message(const message& msg);
  };

}  // namespace other

OTHER_DRIVER(other::terminal)

#endif  // OTHER_TERMINAL_TERMINAL_HPP
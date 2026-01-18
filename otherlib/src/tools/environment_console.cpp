/**
 * \file tools/environment_console.cpp
 **/
#include "tools/environment_console.hpp"

#include "lua/lua_script.hpp"

namespace other {

  std::thread::id environment_console::console_thread_id = std::thread::id{};

  std::atomic<bool> environment_console::console_initialized = false;

  std::mutex environment_console::input_mutex;
  std::atomic<bool> environment_console::input_waiting = false;
  std::queue<console_input> environment_console::input_queue = {};

  size_t environment_console::history_cursor = 0;
  std::vector<console_input> environment_console::history_lines = {};
  std::vector<console_input> environment_console::long_term_history_lines = {};

  size_t environment_console::max_history_lines = 100;
  std::array<char, environment_console::kInputBufferSize> environment_console::input_buffer = { 0 };
  lua_script* environment_console::console_lua_script = nullptr;

  void environment_console::initialize(lua_script* console_script) {
    if (console_initialized) {
      CORE_LOG_ERROR("Environment console is already initialized.");
      return;
    }

    console_thread_id = std::this_thread::get_id();
    console_lua_script = console_script;

    /// run .envrc script if it exists
    if (console_lua_script != nullptr) {
      console_lua_script->call_hook_function_if_exists("__environment_console_init_hook");
    }

    console_initialized = true;
  }

  void environment_console::poll() {
    if (!console_initialized) {
      return;
    }
    if (std::this_thread::get_id() != console_thread_id) {
      CORE_LOG_ERROR("Environment console polled from incorrect thread.");
      return;
    }

    if (input_waiting) {
      while (!input_queue.empty()) {
        console_input input = {};
        {
          std::lock_guard lock(input_mutex);
          input = std::move(input_queue.front());
          input_queue.pop();
        }

        bool is_command = false;
        if (console_lua_script != nullptr) {
          is_command = console_lua_script->call_function<bool>("is_command", input.input_text);
        }

        int32_t type_flags = input.message_type;
        if (is_command) {
          type_flags |= CONSOLE_MESSAGE_COMMAND;
        }

        history_lines.push_back({
          .input_text = std::string{ input.input_text },
          .message_type = type_flags,
          .timestamp = input.timestamp,
        });

        if (is_command && console_lua_script != nullptr) {
          console_lua_script->call_function<void>("handle_console_command", std::string(input.input_text));
        } else {
        }
      }
    }
  }

  void environment_console::push_message(const console_input& input) {
    if (!console_initialized) {
      return;
    }
    if (std::this_thread::get_id() != console_thread_id) {
      CORE_LOG_ERROR("Environment console push_message called from incorrect thread.");
      return;
    }

    history_lines.push_back(input);
    long_term_history_lines.push_back(input);
    ++history_cursor;

    if (history_lines.size() > max_history_lines) {
      history_lines.erase(history_lines.begin());
    }
  }

  void environment_console::submit_console_text(const std::string_view text, console_message_type type, system_timepoint time_point) {
    if (!console_initialized) {
      return;
    }
    if (text.empty()) {
      return;
    }
    if (text.length() >= kInputBufferSize) {
      CORE_LOG_ERROR("Environment console input text exceeds maximum length of {} characters.", kInputBufferSize - 1);
      return;
    }

    std::lock_guard lock(input_mutex);
    input_queue.push({
      .input_text = std::string{ text },
      .message_type = type,
      .timestamp = time_point,
    });
    input_waiting = true;
  }

  void environment_console::clear_console_output() {
    if (!console_initialized) {
      return;
    }
    if (std::this_thread::get_id() != console_thread_id) {
      CORE_LOG_ERROR("Environment console clear_console_output called from incorrect thread.");
      return;
    }

    history_lines.clear();
    history_cursor = 0;
  }

  void environment_console::move_history_cursor(history_move move) {
    if (move == HISTORY_MOVE_BACK) {
      if (history_cursor > 0) {
        --history_cursor;
      }
    } else if (move == HISTORY_MOVE_FORWARD) {
      if (history_cursor < history_lines.size()) {
        ++history_cursor;
      }
    }
  }

}  // namespace other
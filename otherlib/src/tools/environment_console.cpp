/**
 * \file tools/environment_console.cpp
 **/
#include "tools/environment_console.hpp"

#include "core/timer.hpp"

#include "lua/lua_script.hpp"

namespace other {

  std::vector<console_input> environment_console::history_lines = {};
  size_t environment_console::max_history_lines = 100;
  std::array<char, environment_console::kInputBufferSize> environment_console::input_buffer = { 0 };
  lua_script* environment_console::console_lua_script;

  void environment_console::initialize(lua_script* console_script) {
    console_lua_script = console_script;

    /// run .envrc script if it exists
    if (console_lua_script != nullptr) {
      console_lua_script->call_hook_function_if_exists("__environment_console_init_hook");
    }
  }

  void environment_console::push_message(const console_input& input) {
    history_lines.push_back(input);
    if (history_lines.size() > max_history_lines) {
      history_lines.erase(history_lines.begin());
    }
  }

  void environment_console::submit_console_text(const std::string_view text, console_message_type type, system_timepoint time_point) {
    if (text.empty()) {
      return;
    }

    bool is_command = false;
    if (console_lua_script != nullptr) {
      is_command = console_lua_script->call_function<bool>("is_command", text);
    }

    int32_t type_flags = type;
    if (is_command) {
      type_flags |= CONSOLE_MESSAGE_COMMAND;
    }

    history_lines.push_back({
      .input_text = std::string{ text },
      .message_type = type_flags,
      .timestamp = time_point,
    });

    if (is_command && console_lua_script != nullptr) {
      console_lua_script->call_function<void>("handle_console_command", std::string(text));
    } else {
    }
  }

}  // namespace other
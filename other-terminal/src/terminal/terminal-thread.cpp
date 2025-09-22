/**
 * \file terminal/terminal-thread.cpp
 **/
#include "terminal-thread.hpp"

namespace other {

  void terminal_thread::send_message_to_terminal(const message& msg) {
    OTHER_ASSERT(terminal_handle != nullptr, "Terminal handle is null");
  }

  void terminal_thread::pump_thread() {
    if (!command_block.command_queue.empty()) {
      exit_code ec = executor.execute(command_block);
      if (ec != exit_code::SUCCESS) {
        error_alert_msg error_msg;
        error_msg.error_code = static_cast<uint64_t>(ec);
        switch (ec) {
          case exit_code::INVALID_COMMAND: error_msg.error_message = "Invalid command"; break;
          case exit_code::INVALID_OPCODE: error_msg.error_message = "Invalid opcode"; break;
          case exit_code::INVALID_ARGUMENT: error_msg.error_message = "Invalid argument"; break;
          case exit_code::MISSING_ARGUMENT: error_msg.error_message = "Missing argument"; break;
          default: error_msg.error_message = "Command execution failed"; break;
        }

        message err_msg;
        err_msg.header = { error_msg.category, error_msg.id };
        err_msg.data = error_msg.build();

        thread_send_message(std::move(err_msg));
      }

      command_block.command_queue = {};
      command_block.argument_queue = {};
    }
  }

  void terminal_thread::handle_command(const other_command_msg& msg) {
    const command& cmd = msg.cmd;

    command_block.command_queue.push(cmd);
    for (const auto& arg : msg.args) {
      command_block.argument_queue.push(arg);
    }
  }

  void terminal_thread::handle_command_block(const other_command_block_msg& msg) {
  }

}  // namespace other
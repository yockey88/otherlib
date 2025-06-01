/**
 * \file terminal/terminal_thread.cpp
 **/
#include "terminal_thread.hpp"

namespace other {

  void terminal_thread::pump_thread() {
    if (!command_block.command_queue.empty()) {
      exit_code ec = executor.execute(command_block);
      if (ec != exit_code::SUCCESS) {
        CORE_LOG_ERROR("Command execution failed with error code: {}", static_cast<int>(ec));
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
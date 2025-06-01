/**
 * \file terminal/terminal_thread.hpp
 **/
#ifndef OTHER_TERMINAL_TERMINAL_THREAD_HPP
#define OTHER_TERMINAL_TERMINAL_THREAD_HPP

#include "command/command.hpp"
#include "command/command_compiler.hpp"
#include "command/command_executor.hpp"
#include "command/command_parser.hpp"
#include "thread/thread.hpp"

namespace other {

  class terminal;

  class terminal_thread : public thread {
   public:
    terminal_thread(terminal* term)
        : thread("terminal-thread"), terminal_handle(term) {}
    ~terminal_thread() override = default;

   private:
    terminal* terminal_handle = nullptr;

    command_executor executor;

    command_block command_block;

    // sol::state lua_state;

    // void source_file(const filepath& file_path);
    // void SourceCmdFile(const filepath& file_path);
    // void SourceLuaFile(const filepath& file_path);
    // void SourcePythonFile(const filepath& file_path);

    void pump_thread() override;

    void handle_command(const other_command_msg& msg) override;
    void handle_command_block(const other_command_block_msg& msg) override;
  };

}  // namespace other

#endif  // OTHER_TERMINAL_TERMINAL_THREAD_HPP
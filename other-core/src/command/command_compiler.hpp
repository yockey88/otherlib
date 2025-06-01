/**
 * \file terminal/command_compiler.hpp
 **/
#ifndef OTHER_CORE_COMMAND_COMMAND_COMPILER_HPP
#define OTHER_CORE_COMMAND_COMMAND_COMPILER_HPP

#include "core/registers.hpp"

#include "command/command.hpp"

namespace other {

  // class Terminal;

  class command_compiler {
   public:
    command_compiler() = default;
    ~command_compiler() = default;

    std::vector<std::pair<command, std::vector<address_t>>> compile(const std::vector<raw_command>& cmds);
    std::pair<command, std::vector<address_t>> compile(const raw_command& cmd);
  };

}  // namespace other

#endif  // OTHER_CORE_COMMAND_COMMAND_COMPILER_HPP
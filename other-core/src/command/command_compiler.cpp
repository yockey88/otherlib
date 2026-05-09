/**
 * \file command/command_compiler.cpp
 **/
#include "command/command_compiler.hpp"

#include "core/logger.hpp"

// #include "environment/terminal.hpp"

namespace other {

  std::vector<std::pair<command, std::vector<address_t>>> command_compiler::compile(const std::vector<raw_command>& cmds) {
    std::vector<std::pair<command, std::vector<address_t>>> compiled_commands;
    compiled_commands.reserve(cmds.size());

    for (auto itr = cmds.begin(); itr != cmds.end(); ++itr) {
      std::pair<command, std::vector<address_t>> compiled_command = compile(*itr);
      compiled_commands.push_back(compiled_command);
    }

    return compiled_commands;
  }

  std::pair<command, std::vector<address_t>> command_compiler::compile(const raw_command& cmd) {
    op_code opcode = op_code::INVALID_OP;
    uint64_t name_hash = FNV(cmd.name);
    for (auto& cmd : kAvailableCommands) {
      if (cmd.hash == name_hash) {
        opcode = cmd.opcode;
        break;
      }
    }

    if (opcode == op_code::INVALID_OP) {
      return { command{ op_code::INVALID_OP }, {} };
    }

    auto itr = std::ranges::find_if(kAvailableCommands, [opcode](const other_command& cmd) { return cmd.opcode == opcode; });
    /// this is impossible because we already checked for invalid opcode
    OTHER_ASSERT(itr != kAvailableCommands.end(), "Failed to find command in available commands");

    if (cmd.args.size() < itr->num_args.min || cmd.args.size() > itr->num_args.max) {
      return { command{ op_code::INVALID_OP }, {} };
    }

    command command;
    command.opcode = opcode;
    command.num_args = cmd.args.size();

    std::vector<address_t> arg_addresses;
    arg_addresses.reserve(command.num_args);

    // registers& registers = arena::GetRegisters();
    // for (auto itr = raw_command.args.begin(); itr != raw_command.args.end(); ++itr) {
    //   address_t addr = registers.Write(*itr);
    //   OTHER_ASSERT(addr != address::kNullAddress, "Failed to write argument to register");
    //   arg_addresses.push_back(addr);
    // }

    return { command, arg_addresses };
  }

}  // namespace other
/**
 * \file command/command.hpp
 **/
#ifndef OTHER_CORE_COMMAND_COMMAND_HPP
#define OTHER_CORE_COMMAND_COMMAND_HPP

#include <cstdint>
#include <queue>
#include <string_view>

#include "core/fnv.hpp"

namespace other {

  struct raw_command {
    std::string name;
    std::vector<std::string> args;
  };

  /**
   * Command Categories
   *
   *  - control commands : commands for controlling the terminal
   *      ex: help, clear, exit, call, lua, py
   *  - file commands : commands for interacting with the filesystem
   *      ex: ls, pwd, source
   *  - module commands : commands for interacting with modules
   *      ex: load, unload
   *  - net commands : commands for networking
   *      ex: listen, connect
   *  - debug commands : commands for debugging
   *      ex: echo
   **/

  enum command_category : uint8_t {
    CONTROL_CMD = 0x01,
    FILE_CMD = 0x02,
    MODULE_CMD = 0x03,
    NET_CMD = 0x04,
  };

  enum control_command : uint8_t {
    HELP_CMD = 0x01,
    CLEAR_CMD = 0x02,
    EXIT_CMD = 0x03,
    CALL_CMD = 0x04,
    LUA_CALL_CMD = 0x05,
    CREATE_CMD = 0x06,
    ECHO_CMD = 0x017,
  };

  enum file_command : uint8_t {
    LS_CMD = 0x01,
    PWD_CMD = 0x02,
    SOURCE_CMD = 0x03,
    MOUNT_CMD = 0x04,
  };

  enum module_command : uint8_t {
    LOAD_CMD = 0x01
  };

  enum net_command : uint8_t {
    LISTEN_CMD = 0x01,
    CONNECT_CMD = 0x02
  };

#define COMMAND(cat, val) (cat << 8 | val)
#define CATEGORY(op) ((op & 0xFF00) >> 8)
#define VALUE(op) (op & 0x00FF)

  enum op_code : uint16_t {
    NO_OP = 0,

    // control commands
    HELP_OP = COMMAND(CONTROL_CMD, HELP_CMD),
    CLEAR_OP = COMMAND(CONTROL_CMD, CLEAR_CMD),
    EXIT_OP = COMMAND(CONTROL_CMD, EXIT_CMD),
    CALL_OP = COMMAND(CONTROL_CMD, CALL_CMD),
    LUA_CALL_OP = COMMAND(CONTROL_CMD, LUA_CALL_CMD),
    CREATE_OP = COMMAND(CONTROL_CMD, CREATE_CMD),
    ECHO_OP = COMMAND(CONTROL_CMD, ECHO_CMD),

    /// file commands
    LS_OP = COMMAND(FILE_CMD, LS_CMD),
    PWD_OP = COMMAND(FILE_CMD, PWD_CMD),
    SOURCE_OP = COMMAND(FILE_CMD, SOURCE_CMD),
    MOUNT_OP = COMMAND(FILE_CMD, MOUNT_CMD),

    /// module commands
    LOAD_OP = COMMAND(MODULE_CMD, LOAD_CMD),

    /// net commands
    LISTEN_OP = COMMAND(NET_CMD, LISTEN_CMD),
    CONNECT_OP = COMMAND(NET_CMD, CONNECT_CMD),

    NUM_OPS,
    INVALID_OP = NUM_OPS
  };

  struct arity_range {
    uint16_t min;
    uint16_t max;

    constexpr arity_range(uint16_t arity) : min(arity), max(arity) {}
    constexpr arity_range(uint16_t min, uint16_t max) : min(min), max(max) {}
    constexpr auto operator<=>(const arity_range&) const = default;
  };

  struct other_command {
    op_code opcode;

    std::string_view name;
    uint64_t hash;

    arity_range num_args;

    std::string_view description;
    std::string_view meta_var_name;

    constexpr other_command(op_code code, const std::string_view name, uint16_t num_args, const std::string_view description, const std::string_view meta_var_name)
        : opcode(code), name(name), hash(FNV(name)), num_args(num_args), description(description), meta_var_name(meta_var_name) {}
    constexpr other_command(op_code code, const std::string_view name, uint16_t min_args, uint16_t max_args, const std::string_view description, const std::string_view meta_var_name)
        : opcode(code), name(name), hash(FNV(name)), num_args(min_args, max_args), description(description), meta_var_name(meta_var_name) {}

    constexpr auto operator<=>(const other_command&) const = default;
  };

  constexpr static std::array kAvailableCommands = {
    /// control commands
    other_command{ op_code::HELP_OP, "help", 0, "Print the help menu", "" },
    other_command{ op_code::CLEAR_OP, "clear", 0, "Clear the terminal screen", "" },
    other_command{ op_code::EXIT_OP, "exit", 0, "Exit the terminal", "" },
    other_command{ op_code::CALL_OP, "call", 1, 256, "Call a function", "function" },
    other_command{ op_code::LUA_CALL_OP, "lua", 1, 256, "Call a lua function", "function" },

    /// file commands
    other_command{ op_code::LS_OP, "ls", 0, "Lists the mounted directories", "directory" },
    other_command{ op_code::PWD_OP, "pwd", 0, "Print the current working directory", "" },
    other_command{ op_code::SOURCE_OP, "source", 1, "Source a file", "file" },
    other_command{ op_code::MOUNT_OP, "mount", 1, "Mount a directory", "directory" },

    /// module commands
    other_command{ op_code::LOAD_OP, "load", 1, "Load a module", "module" },

    /// net commands
    other_command{ op_code::LISTEN_OP, "listen", 1, "Listen for incoming connections", "port" },
    other_command{ op_code::CONNECT_OP, "connect", 1, "Connect to a remote host", "host" },

    /// debug commands
    other_command{ op_code::ECHO_OP, "echo", 1, "Print a message", "message" },

    /// other commands
    other_command{ op_code::CREATE_OP, "create", 1, "Create a new object", "object" }
  };

  /**
   * command bytecode
   *     - args are stored in registers and their addresses are indexed into by the value in the corresponding arg field
   * TODO: how to better handle arguments st we are not limited by the number of registers?
   *       (we could implement a custom arbitrary size integer and determine how many bits to parse using the num_args field)
   * |--------------------|
   * | 16 bits | 16 bits  |
   * |--------------------|
   * |  opcode | num-args |
   * |--------------------|
   **/
  struct command {
    union {
      uint32_t data = 0;
      struct {
        uint16_t opcode;
        uint16_t num_args;
      };
    };
  };

  struct command_block {
    std::string block_name;
    std::queue<command> command_queue;
    std::queue<uint64_t> argument_queue;  // addresses of arguments in the registers
  };

}  // namespace other

#endif  // OTHER_CORE_CORE_COMMAND_HPP
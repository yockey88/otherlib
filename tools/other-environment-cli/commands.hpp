/**
 * \file other-environment-cli/commands.hpp
 **/
#ifndef OTHER_ENVIRONMENT_CLI_COMMANDS_HPP
#define OTHER_ENVIRONMENT_CLI_COMMANDS_HPP

#include <string>
#include <vector>

#include "core/defines.hpp"

struct oecli_command {
  std::string name;
  std::vector<std::string> args;
};

other::opt<oecli_command> parse_command_line_arguments(int argc, char* argv[]);

int handle_command(const oecli_command& command);

#endif  // OTHER_ENVIRONMENT_CLI_COMMANDS_HPP
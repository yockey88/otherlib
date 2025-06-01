/**
 * \file other-terminal.cpp
 **/
#include "terminal/terminal.hpp"

#include "other.hpp"

using other::terminal;

exit_code other_main(const command_line& cmdline, const config_table& config) {
  /// \todo: fix this
  // terminal{}.run(cmdline, config);
  return exit_code::SUCCESS;
}
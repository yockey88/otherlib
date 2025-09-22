/**
 * \file other-terminal.cpp
 **/
#include "terminal/terminal.hpp"

#include "other.hpp"

exit_code other_main(const command_line& cmdline, const config_table& config) {
  PROFILE_SECTION("terminal--other_main");
  other::driver* terminal = create_driver(&config);
  if (!terminal) {
    CORE_LOG_ERROR("Failed to create terminal driver");
    return exit_code::FAILURE;
  }

  terminal->initialize();
  terminal->run();
  terminal->shutdown();

  destroy_driver(terminal);
  return exit_code::SUCCESS;
}
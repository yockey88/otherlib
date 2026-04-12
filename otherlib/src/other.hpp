/**
 * @file other.hpp
 */
#ifndef OTHERLIB_OTHER_HPP
#define OTHERLIB_OTHER_HPP

#include "core/command_line.hpp"
#include "core/logger.hpp"

#include "driver/driver.hpp"
#include "driver/subsystem_registry.hpp"

namespace other {

  struct other_plugin_argv;

  int entry(int argc, char* argv[]);
  void shutdown_subsystems();

  using load_config_result = std::tuple<bool, config_table, command_line>;
  load_config_result read_command_line_and_config(int argc, char* argv[]);

}  // namespace other

#endif  // OTHERLIB_OTHER_HPP
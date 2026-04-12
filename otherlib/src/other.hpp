/**
 * @file other.hpp
 */
#ifndef OTHERLIB_OTHER_HPP
#define OTHERLIB_OTHER_HPP

#include "core/command_line.hpp"
#include "core/defines.hpp"
#include "core/logger.hpp"

#include "driver/driver.hpp"

using other::command_line;
using other::config_table;
using other::driver;
using other::exit_code;

namespace other {

  struct other_plugin_argv;

  void register_log_sinks(const config_table& config);
  void shutdown_subsystems();

  int entry(int argc, char* argv[]);

  using load_config_result = std::tuple<bool, config_table, command_line>;
  load_config_result read_command_line_and_config(int argc, char* argv[]);

}  // namespace other

#endif  // OTHERLIB_OTHER_HPP
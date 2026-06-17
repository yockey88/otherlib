/**
 * @file other.hpp
 */
#ifndef OTHERLIB_OTHER_HPP
#define OTHERLIB_OTHER_HPP

// clang-format off
#include "core/build_config.hpp"
#include "core/defines.hpp"
// clang-format on

#include "core/logger.hpp"

#include "driver/driver.hpp"
#include "driver/driver_kernel.hpp"
#include "driver/subsystem_registry.hpp"
#include "plugin/plugin.hpp"

namespace other {

  struct other_plugin_argv;

  int entry();                        // default
  int entry(int argc, char* argv[]);  // if application expects config file/command line arguments
  void shutdown_subsystems();

  using load_config_result = std::tuple<bool, config_table, command_line>;
  load_config_result read_command_line_and_config(int argc, char* argv[]);

}  // namespace other

#endif  // OTHERLIB_OTHER_HPP
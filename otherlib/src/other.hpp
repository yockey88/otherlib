/**
 * @file other.hpp
 */
#ifndef OTHER_OTHERLIB_OTHER_HPP
#define OTHER_OTHERLIB_OTHER_HPP

#include <iostream>
#include <print>

#include "core/command_line.hpp"
#include "core/defines.hpp"
#include "core/logger.hpp"
#include "core/version.hpp"
#include "driver/driver.hpp"
#include "plugin/plugin.hpp"

using other::command_line;
using other::config_table;
using other::driver;

namespace other {

  struct other_plugin_argv;

  void initialize_primary_arena();
  void register_log_sinks(const config_table& config);
  void shutdown_subsystems();

  int entry(int argc, char* argv[]);

}  // namespace other

#ifdef OTHER_APPLICATION

  #ifdef OTHER_ENVIRONMENT_WINDOWS
    #include <windows.h>
  #endif

  #ifndef MAIN_DEFINED
    #define MAIN_DEFINED
int main(int argc, char* argv[]) {
  return other::entry(argc, argv);
}
  #endif  // MAIN_DEFINED
#endif    // OTHER_APPLICATION

#endif  // OTHER_OTHERLIB_OTHER_HPP
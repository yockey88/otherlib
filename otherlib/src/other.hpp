/**
 * \file other.hpp
 **/
#ifndef OTHER_HPP
#define OTHER_HPP

#include "core/command_line.hpp"
#include "core/defines.hpp"
#include "core/logger.hpp"
#include "driver/driver.hpp"
#include "plugin/plugin.hpp"

using other::command_line;
using other::config_table;
using other::driver;

#if defined(OTHER_DEBUG_BUILD) || defined(OTHER_DEBUG_AS_BUILD)
  #define CATCH_RUNTIME_ERROR(e) OTHER_ASSERT(false, "Runtime error: {}", e.what());
  #define CATCH_EXCEPTION(e) OTHER_ASSERT(false, "Exception: {}", e.what());
  #define CATCH_UNKNOWN_EXCEPTION() OTHER_ASSERT(false, "Unknown exception occurred.");
#else
  #define CATCH_RUNTIME_ERROR(e) CORE_LOG_ERROR("Runtime error: {}", e.what());
  #define CATCH_EXCEPTION(e) CORE_LOG_ERROR("Exception: {}", e.what());
  #define CATCH_UNKNOWN_EXCEPTION() CORE_LOG_ERROR("Unknown exception occurred.");
#endif

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
int main(int argc, char* argv[]) {
  return other::entry(argc, argv);
}
#endif

#endif  // OTHER_HPP
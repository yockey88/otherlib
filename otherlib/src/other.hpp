/**
 * @file other.hpp
 */
#ifndef OTHERLIB_OTHER_HPP
#define OTHERLIB_OTHER_HPP

#include <iostream>
#include <print>

#include "core/command_line.hpp"
#include "core/defines.hpp"
#include "core/logger.hpp"
#include "core/version.hpp"

#include "script/scripting_environment.hpp"

#include "scene/scene.hpp"
#include "scene/scene_serialization.hpp"

#include "driver/driver.hpp"
#include "plugin/plugin.hpp"
#include "project/project_serialization.hpp"

using other::command_line;
using other::config_table;
using other::driver;
using other::exit_code;

namespace other {

  struct other_plugin_argv;

  void initialize_primary_arena();

  void bind_physics_environment(const config_table& config);
  void bind_primary_scripting_environment(const config_table& config);
  void bind_environment_scripts();
  void cleanup_scripting_environment();
  void cleanup_physics_environment();

  void register_log_sinks(const config_table& config);
  void shutdown_subsystems();

  void initialize_other_environment();
  void initialize_other_environment(int argc, char* argv[]);
  void shutdown_other_environment();

  int entry(int argc, char* argv[]);

}  // namespace other

#endif  // OTHERLIB_OTHER_HPP
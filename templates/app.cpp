/**
 * \file ${project-name}.cpp
 **/
 #include "${project-name}_driver.hpp"

#include "other.hpp"

using other::command_line;
using other::config_table;
using other::exit_code;

other::exit_code other_main(const other::command_line& cmd, const other::config_table& config) {
  other::driver* runtime = create_driver(&config);
  if (!runtime) {
    CORE_LOG_ERROR("Failed to create ${project-name} driver");
    return other::exit_code::FAILURE;
  }

  runtime->initialize(cmd);
  runtime->run();
  runtime->shutdown();

  destroy_driver(runtime);
  return other::exit_code::SUCCESS;
}
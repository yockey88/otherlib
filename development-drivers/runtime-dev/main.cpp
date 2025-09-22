/**
 * \file editor-dev/main.cpp
 **/
#include "other.hpp"
#include "runtime.hpp"


other::exit_code other_main(const other::command_line& cmd, const other::config_table& config) {
  PROFILE_SECTION("simulation--other_main");
  other::driver* runtime = create_driver(&config);
  if (!runtime) {
    CORE_LOG_ERROR("Failed to create simulation driver");
    return other::exit_code::FAILURE;
  }

  runtime->initialize();
  runtime->run();
  runtime->shutdown();

  destroy_driver(runtime);
  return other::exit_code::SUCCESS;
}
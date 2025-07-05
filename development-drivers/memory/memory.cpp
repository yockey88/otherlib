/**
 * \file memory/memory.cpp
 **/
#include "memory_sandbox.hpp"
#include "other.hpp"

exit_code other_main(const command_line& cmd, const config_table& config) {
  PROFILE_SECTION("rendering-dev--other_main");

  other::driver* renderer_driver = create_driver(&config);
  if (!renderer_driver) {
    CORE_LOG_ERROR("Failed to create renderer driver");
    return exit_code::FAILURE;
  }

  renderer_driver->initialize();
  renderer_driver->run();
  renderer_driver->shutdown();

  destroy_driver(renderer_driver);
  return exit_code::SUCCESS;
}
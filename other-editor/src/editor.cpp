/**
 * \file editor.cpp
 **/
#include "editor_driver.hpp"
#include "other.hpp"

using namespace other;

exit_code other_main(const command_line& cmd, const config_table& config) {
  auto* driver = create_driver(&config);
  if (!driver) {
    CORE_LOG_ERROR("Failed to create editor driver.");
    return exit_code::FAILURE;
  }

  driver->initialize(cmd);
  driver->run();
  driver->shutdown();

  destroy_driver(driver);
  return exit_code::SUCCESS;
}
/**
 * \file rendering-dev/renderer.cpp
 **/
#include "core/defines.hpp"

#include "other.hpp"
#include "renderer_driver.hpp"

exit_code other_main(const command_line& cmd, const config_table& config) {
  PROFILE_SECTION("rendering-dev--other_main");
  other::renderer_driver* driver = new other::renderer_driver(config);
  if (!driver) {
    CORE_LOG_ERROR("Failed to create renderer driver");
    return exit_code::FAILURE;
  }

  driver->initialize(cmd);
  driver->run();
  driver->on_shutdown();

  delete driver;
  return exit_code::SUCCESS;
}
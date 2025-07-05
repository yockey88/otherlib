/**
 * \file rendering-dev/renderer.cpp
 **/
#include "core/defines.hpp"

#include "other.hpp"
#include "renderer_driver.hpp"

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
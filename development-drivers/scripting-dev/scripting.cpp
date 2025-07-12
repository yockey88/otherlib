/**
 * \file scripting-dev/renderer.cpp
 **/
#include "core/defines.hpp"

#include "other.hpp"
#include "scripting_driver.hpp"

exit_code other_main(const command_line& cmd, const config_table& config) {
  PROFILE_SECTION("rendering-dev--other_main");
  other::driver* scripting_driver = create_driver(&config);
  if (!scripting_driver) {
    CORE_LOG_ERROR("Failed to create scripting driver");
    return exit_code::FAILURE;
  }

  scripting_driver->initialize();
  scripting_driver->run();
  scripting_driver->shutdown();

  destroy_driver(scripting_driver);
  return exit_code::SUCCESS;
}
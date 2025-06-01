/**
 * \file other_driver.cpp
 **/
#include <print>

#include "other.hpp"

exit_code other_main(const command_line& cmd, const config_table& config) {
  CORE_LOG_INFO("Configuration loaded successfully from: {}", cmd.config_file);
  auto [driver_instance, driver_name] = driver::create(config);

  if (driver_instance != nullptr) {
    CORE_LOG_INFO("Running Other Environment driver '{}'", driver_name);
    driver_instance->initialize();
    driver_instance->run();
    driver_instance->shutdown();
  }

  driver::destroy(driver_name, driver_instance);
  CORE_LOG_INFO("Other Environment driver '{}' has finished unloading.", driver_name);

  return exit_code::SUCCESS;
}
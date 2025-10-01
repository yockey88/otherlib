/**
 * \file main.cpp
 **/
#include "other.hpp"
#include "simulation_driver.hpp"

other::exit_code other_main(const other::command_line& cmd, const other::config_table& config) {
  PROFILE_SECTION("simulation--other_main");
  other::driver* simulation_driver = create_driver(&config);
  if (!simulation_driver) {
    CORE_LOG_ERROR("Failed to create simulation driver");
    return other::exit_code::FAILURE;
  }

  simulation_driver->initialize(cmd);
  simulation_driver->run();
  simulation_driver->shutdown();

  destroy_driver(simulation_driver);
  return other::exit_code::SUCCESS;
}
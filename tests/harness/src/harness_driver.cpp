/**
 * \file tests/harness/src/harness_driver.cpp
 **/
#include "harness_driver.hpp"

#include "core/logger.hpp"

#include "network_scenario.hpp"
#include "soak_scenario.hpp"

namespace other {

  scope<harness_scenario> harness_driver::create_scenario(std::string_view scenario_name) {
    if (scenario_name == "soak") {
      return make_scope<soak_scenario>();
    }
    if (scenario_name == "network") {
      return make_scope<network_scenario>();
    }

    /// future scenarios (fuzzing, replay, stress, ...) register here
    CORE_LOG_ERROR("[HARNESS] unknown scenario '{}', falling back to 'soak'", scenario_name);
    return make_scope<soak_scenario>();
  }

  void harness_driver::on_initialize() {
    std::string scenario_name = configuration().get_value<std::string>("harness.scenario", "soak");
    active_scenario = create_scenario(scenario_name);
    CORE_LOG_INFO("[HARNESS] running scenario '{}'", active_scenario->name());
    active_scenario->initialize(*this, configuration());
  }

  void harness_driver::update_running() {
    if (active_scenario == nullptr || shutdown_requested_by_scenario) {
      return;
    }

    if (!active_scenario->update(*this)) {
      shutdown_requested_by_scenario = true;
      CORE_LOG_INFO("[HARNESS] scenario '{}' finished, requesting shutdown", active_scenario->name());
      request_shutdown();
    }
  }

  void harness_driver::on_scene_activated(natural_t scene_id) {
    if (active_scenario != nullptr) {
      active_scenario->scene_activated(*this, scene_id);
    }
  }

  void harness_driver::on_shutdown() {
    if (active_scenario == nullptr) {
      return;
    }

    bool passed = active_scenario->finalize(*this);
    CORE_LOG_INFO("[HARNESS] scenario '{}' result: {}", active_scenario->name(), passed ? "PASS" : "FAIL");
    active_scenario = nullptr;
  }

}  // namespace other

OTHER_DRIVER(other::harness_driver)

/**
 * \file tests/harness/src/harness_driver.hpp
 *
 * a minimal static driver that hosts exactly one harness scenario (config key
 * `harness.scenario`). the driver stays generic; all behavior lives in scenarios.
 **/
#ifndef OTHER_TESTS_HARNESS_DRIVER_HPP
#define OTHER_TESTS_HARNESS_DRIVER_HPP

#include "driver/driver.hpp"

#include "harness_scenario.hpp"

namespace other {

  class OTHER_CLASS harness_driver : public driver {
   public:
    harness_driver(const command_line& cmd, const config_table& config)
        : driver(cmd, config) {}
    ~harness_driver() override {}

    void on_initialize() override;
    void update_running() override;
    void on_scene_activated(natural_t scene_id) override;
    void on_shutdown() override;

   private:
    scope<harness_scenario> active_scenario = nullptr;
    bool shutdown_requested_by_scenario = false;

    static scope<harness_scenario> create_scenario(std::string_view scenario_name);
  };

}  // namespace other

#endif  // OTHER_TESTS_HARNESS_DRIVER_HPP

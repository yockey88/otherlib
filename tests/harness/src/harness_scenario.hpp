/**
 * \file tests/harness/src/harness_scenario.hpp
 *
 * the test harness runs exactly one scenario per process inside a real engine driver.
 * scenarios are selected by config key `harness.scenario`; today only "soak" exists,
 * but the interface is the seam for future harness work (fuzzing, replay, stress, ...).
 **/
#ifndef OTHER_TESTS_HARNESS_SCENARIO_HPP
#define OTHER_TESTS_HARNESS_SCENARIO_HPP

#include <string_view>

#include "core/config_table.hpp"
#include "core/defines.hpp"

namespace other {

  class driver;

  class harness_scenario {
   public:
    virtual ~harness_scenario() = default;

    virtual std::string_view name() const = 0;

    /// called once from the driver's on_initialize hook
    virtual void initialize(driver& host, const config_table& config) = 0;

    /// called once per frame while the driver is RUNNING; return false when the
    ///  scenario is finished and the driver should begin shutting down
    virtual bool update(driver& host) = 0;

    virtual void scene_activated(driver& host, natural_t scene_id) {}

    /// called from the driver's on_shutdown hook; returns true if the scenario passed.
    ///  the process exit code cannot carry the verdict (other_main returns SUCCESS for
    ///  any clean run), so scenarios must persist their verdict in a report file
    virtual bool finalize(driver& host) = 0;
  };

}  // namespace other

#endif  // OTHER_TESTS_HARNESS_SCENARIO_HPP

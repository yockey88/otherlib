/**
 * \file tests/harness/src/harness_scenario.hpp
 *
 * runs exactly one scenario per process inside a real engine driver, selected by config key
 *  `harness.scenario`; today only "soak" exists — seam for future work (fuzzing, replay, stress)
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

    /// returns true if the scenario passed; exit code can't carry the verdict (other_main
    ///  always returns SUCCESS), so scenarios must persist it in a report file
    virtual bool finalize(driver& host) = 0;
  };

}  // namespace other

#endif  // OTHER_TESTS_HARNESS_SCENARIO_HPP

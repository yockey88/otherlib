/**
 * \file driver/systems/job_driver_system.cpp
 **/
#include "driver/systems/job_driver_system.hpp"

#include "driver/driver.hpp"
#include "driver/systems/network_system.hpp"

namespace other {

  void job_driver_system::initialize(driver_kernel* kernel) {
    OTHER_ASSERT(kernel != nullptr, "Driver kernel is null in job driver system initialization.");
    jobs = make_scope<job_system>(sibling<network_system>(*kernel).io_context());
    jobs->initialize(get_driver().configuration());
  }

  void job_driver_system::tick(driver_kernel* kernel, double dt) {
    OTHER_ASSERT(jobs != nullptr, "Job system is not initialized in job driver system tick.");
    jobs->poll();
  }

  void job_driver_system::shutdown(driver_kernel* kernel) {
    OTHER_ASSERT(jobs != nullptr, "Job system is not initialized in job driver system shutdown.");
    jobs->shutdown();
    jobs = nullptr;
  }

}  // namespace other
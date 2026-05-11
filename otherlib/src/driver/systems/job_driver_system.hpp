/**
 * \file driver/systems/job_driver_system.hpp
 **/
#ifndef OTHERLIB_DRIVER_SYSTEMS_JOB_DRIVER_SYSTEM_HPP
#define OTHERLIB_DRIVER_SYSTEMS_JOB_DRIVER_SYSTEM_HPP

#include <string>

#include "core/coroutine.hpp"
#include "core/job_system.hpp"

#include "driver/systems/core_system.hpp"

namespace other {

  class OTHER_CLASS job_driver_system : public core_system<job_driver_system> {
   public:
    job_driver_system(driver* driver_instance)
        : core_system(driver_instance, driver_system_type::JOB_DRIVER_SYSTEM) {}
    virtual ~job_driver_system() = default;

    std::string name() const override { return "Job System"; }

    void initialize(driver_kernel* kernel) override;
    void tick(driver_kernel* kernel, double dt) override;
    void shutdown(driver_kernel* kernel) override;

    inline job_system& get_job_system() {
      OTHER_ASSERT(jobs != nullptr, "Job system is not initialized in job driver system.");
      return *jobs;
    }

    inline void post_coroutine(task&& t) {
      jobs->post_coroutine(std::move(t));
    }

   private:
    scope<job_system> jobs = nullptr;
  };

}  // namespace other

#endif  // OTHERLIB_DRIVER_SYSTEMS_JOB_DRIVER_SYSTEM_HPP
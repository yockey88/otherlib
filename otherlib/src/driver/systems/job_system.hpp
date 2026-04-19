/**
 * \file driver/systems/job_system.hpp
 **/
#ifndef OTHERLIB_DRIVER_SYSTEMS_JOB_SYSTEM_HPP
#define OTHERLIB_DRIVER_SYSTEMS_JOB_SYSTEM_HPP

#include <string>
#include <vector>

#include "core/coroutine.hpp"
#include "core/job.hpp"

#include "driver/systems/core_system.hpp"

namespace other {

  class job_system : public core_system<job_system> {
   public:
    job_system(driver* driver_instance)
        : core_system(driver_instance, driver_system_type::JOB_DRIVER_SYSTEM) {}
    virtual ~job_system() = default;

    std::string name() const override { return "Job System"; }

    void initialize(driver_kernel* kernel) override;
    void tick(driver_kernel* kernel, double dt) override;
    void shutdown(driver_kernel* kernel) override;

    void post_coroutine(task coro);

    template <typename Fn, typename... Args>
      requires std::invocable<Fn, Args...>
    void post_job(const job::descriptor& descriptor, Args&&... args) {
    }

   private:
    struct live_coroutine {
      task handle;
    };

    std::vector<live_coroutine> live_coroutines;
  };

}  // namespace other

#endif  // OTHERLIB_DRIVER_SYSTEMS_JOB_SYSTEM_HPP
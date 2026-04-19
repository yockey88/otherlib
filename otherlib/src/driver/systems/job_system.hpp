/**
 * \file driver/systems/job_system.hpp
 **/
#ifndef OTHERLIB_DRIVER_SYSTEMS_JOB_SYSTEM_HPP
#define OTHERLIB_DRIVER_SYSTEMS_JOB_SYSTEM_HPP

#include <string>
#include <vector>

#include "core/coroutine.hpp"
#include "core/job.hpp"
#include "core/job_graph.hpp"
#include "core/scope.hpp"

#include "driver/driver.hpp"
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

    ref<job> submit(job::descriptor desc, job_graph::work_fn work);
    ref<job> submit(job::descriptor desc, job_graph::work_fn work, std::span<const natural_t> dependencies);
    ref<job> submit_deferred(natural_t trigger_id, job_graph::deferred_factory_fn factory);
    void post_coroutine(task coro);

    template <typename F>
    void post_to_main(F&& work) {
      asio::post(sibling<network_system>(get_driver().get_kernel()).io_context(), std::forward<F>(work));
    }

    template <typename F>
    void post_to_worker(F&& work) {
      asio::post(*pool, std::forward<F>(work));
    }

    void cancel(natural_t id);

    asio::thread_pool& thread_pool() { return *pool; }
    auto worker_executor() { return pool->get_executor(); }

   private:
    struct config_variables {
      constexpr static uint32_t kDefaultWorkCount = 4;
      uint32_t worker_count = 0;
    };
    struct completion_record {
      natural_t id;
      job::status status;
    };
    struct live_coroutine {
      task handle;
    };

    config_variables config;
    job_graph jobs;

    scope<asio::thread_pool> pool;
    std::vector<live_coroutine> live_coroutines;

    mutable std::mutex completion_mutex;
    std::vector<completion_record> pending_completions;

    void initialize(const config_table& cfg);

    void dispatch_ready();
    void dispatch_node(natural_t id);
    void on_job_complete(natural_t id, job::status status);
  };

}  // namespace other

#endif  // OTHERLIB_DRIVER_SYSTEMS_JOB_SYSTEM_HPP
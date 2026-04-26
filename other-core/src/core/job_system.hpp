/**
 * \file core/job_system.hpp
 **/
#ifndef OTHER_CORE_CORE_JOB_SYSTEM_HPP
#define OTHER_CORE_CORE_JOB_SYSTEM_HPP

#include "core/coroutine.hpp"
#include "core/job.hpp"
#include "core/job_graph.hpp"
#include "core/scope.hpp"

namespace other {

  class job_system {
   public:
    job_system(asio::io_context& main_ctx)
        : main_io_context(main_ctx) {}
    ~job_system() = default;

    void initialize(const config_table& cfg);
    void poll();
    void shutdown();

    ref<job> submit(job::descriptor desc, job_graph::work_fn work);
    ref<job> submit(job::descriptor desc, job_graph::work_fn work, std::span<const natural_t> dependencies);
    ref<job> submit_deferred(natural_t trigger_id, job::descriptor desc, job_graph::work_fn work);
    void post_coroutine(task&& coro);

    template <typename F>
    void post_to_main(F&& work) {
      asio::post(main_io_context, std::forward<F>(work));
    }

    template <typename F>
    void post_to_worker(F&& work) {
      OTHER_ASSERT(pool != nullptr, "Thread pool for job system is not initialized.");
      asio::post(*pool, std::forward<F>(work));
    }

    void cancel(natural_t id);

    asio::thread_pool& thread_pool() { return *pool; }
    auto worker_executor() { return pool->get_executor(); }

    inline uint32_t get_num_workers() const { return config.worker_count; }

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

    asio::io_context& main_io_context;

    config_variables config;
    job_graph jobs;

    scope<asio::thread_pool> pool;
    std::vector<live_coroutine> live_coroutines;

    mutable std::mutex completion_mutex;
    std::vector<completion_record> pending_completions;

    void dispatch_ready();
    void dispatch_node(natural_t id);
    void on_job_complete(natural_t id, job::status status);
  };

}  // namespace other

#endif  // OTHER_CORE_CORE_JOB_SYSTEM_HPP
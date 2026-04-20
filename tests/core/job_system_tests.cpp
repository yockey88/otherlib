/**
 * \file core/job_system_tests.cpp
 **/
#include "core/job_system_tests.hpp"

#include "core/job_system.hpp"

namespace other {
  namespace {

    constexpr static size_t kNumWorkers = 4;
    constexpr static std::string_view kConfig =
      R"(
[application.async]
worker_count = {}
)";

  }  // namespace

  TEST_F(job_system_tests, basic_job_execution) {
    asio::io_context main_ctx;
    job_system js(main_ctx);

    config_table cfg = config_table::load_from_source(std::format(kConfig, kNumWorkers));
    js.initialize(cfg);
    ASSERT_EQ(js.get_num_workers(), kNumWorkers);

    std::atomic<int> counter = 0;

    auto job1 = js.submit(
      /// default is medium on main thread
      { .name = "Job 1" },
      [&counter]() {
        CORE_LOG_DEBUG("Job 1 is running on thread {}.", std::this_thread::get_id());
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        counter.fetch_add(1, std::memory_order_relaxed);
        CORE_LOG_DEBUG("Job 1 is completed on thread {}.", std::this_thread::get_id());
      }
    );
    /// main thread job with no other jobs in queue and no dependencies will be dispatched immediately
    EXPECT_EQ(job1->get_status(), job::status::RUNNING);

    main_ctx.poll();
    EXPECT_EQ(job1->get_status(), job::status::COMPLETED);
    EXPECT_EQ(counter.load(std::memory_order_relaxed), 1);

    js.poll();
    js.shutdown();
  }

  TEST_F(job_system_tests, threaded_job) {
    asio::io_context main_ctx;
    job_system js(main_ctx);

    config_table cfg = config_table::load_from_source(std::format(kConfig, kNumWorkers));
    js.initialize(cfg);
    ASSERT_EQ(js.get_num_workers(), kNumWorkers);

    std::atomic<int> counter = 0;

    auto job1 = js.submit(
      { .name = "Threaded Job", .thread_affinity = job::affinity::WORKER_THREAD },
      [&counter]() {
        CORE_LOG_DEBUG("Threaded Job is running on thread {}.", std::this_thread::get_id());
        counter.fetch_add(1, std::memory_order_relaxed);
        CORE_LOG_DEBUG("Threaded Job is completed on thread {}.", std::this_thread::get_id());
      }
    );
    EXPECT_EQ(job1->get_status(), job::status::RUNNING);

    while (counter.load(std::memory_order_relaxed) < 1) {
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
      main_ctx.poll();
      js.poll();
    }

    EXPECT_EQ(job1->get_status(), job::status::COMPLETED);

    js.shutdown();
  }

  TEST_F(job_system_tests, deferred_job_execution) {
    asio::io_context main_ctx;
    job_system js(main_ctx);

    config_table cfg = config_table::load_from_source(std::format(kConfig, kNumWorkers));
    js.initialize(cfg);
    ASSERT_EQ(js.get_num_workers(), kNumWorkers);

    std::atomic<int> counter = 0;

    auto job1 = js.submit(
      { .name = "Job 1" },
      [&counter]() {
        counter.fetch_add(1, std::memory_order_relaxed);
        CORE_LOG_DEBUG("Job 1 is ran on thread {}.", std::this_thread::get_id());
      }
    );
    auto job2 = js.submit_deferred(job1->id, [&counter](job::status s) -> job_graph::deferred_work {
      return {
        { .name = "Deferred Job", .priority = job::priority::HIGH, .thread_affinity = job::affinity::WORKER_THREAD },
        [&counter]() {
          CORE_LOG_DEBUG("Deferred Job is running on thread {}.", std::this_thread::get_id());
          counter.fetch_add(1, std::memory_order_relaxed);
          CORE_LOG_DEBUG("Deferred Job is completed on thread {}.", std::this_thread::get_id());
        }
      };
    });
    EXPECT_EQ(job1->get_status(), job::status::RUNNING);
    EXPECT_EQ(job2->get_status(), job::status::PENDING);

    main_ctx.poll();
    js.poll();
    EXPECT_EQ(job1->get_status(), job::status::COMPLETED);
    EXPECT_EQ(job2->get_status(), job::status::RUNNING);

    while (counter.load(std::memory_order_relaxed) < 2) {
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
      main_ctx.poll();
      js.poll();
    }

    EXPECT_EQ(job2->get_status(), job::status::COMPLETED);

    js.shutdown();
  }

}  // namespace other
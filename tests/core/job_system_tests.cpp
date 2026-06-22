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

    int counter = 0;

    auto job1 = js.submit(
      /// default is medium on main thread
      { .name = "Job 1" },
      [&counter]() {
        counter++;
      });
    ASSERT_NE(job1, nullptr);
    /// main thread job with no other jobs in queue and no dependencies will be dispatched immediately
    EXPECT_EQ(job1->get_status(), job::status::RUNNING);

    main_ctx.poll();
    js.poll();
    EXPECT_EQ(job1->get_status(), job::status::COMPLETED);
    EXPECT_EQ(counter, 1);

    js.shutdown();
  }

  TEST_F(job_system_tests, threaded_job) {
    asio::io_context main_ctx;
    job_system js(main_ctx);

    config_table cfg = config_table::load_from_source(std::format(kConfig, kNumWorkers));
    js.initialize(cfg);
    ASSERT_EQ(js.get_num_workers(), kNumWorkers);

    int counter = 0;

    auto job1 = js.submit(
      { .name = "Threaded Job", .thread_affinity = job::affinity::WORKER_THREAD },
      [&counter]() {
        counter++;
      });
    ASSERT_NE(job1, nullptr);
    EXPECT_EQ(job1->get_status(), job::status::RUNNING);

    uint32_t max_num_polls = 10000;
    uint32_t polls = 0;
    do {
      main_ctx.poll();
      js.poll();

      ++polls;
    } while (!job1->done() && polls < max_num_polls);
    ASSERT_LT(polls, max_num_polls);
    EXPECT_EQ(job1->get_status(), job::status::COMPLETED);
    EXPECT_EQ(counter, 1);

    js.shutdown();
  }

  TEST_F(job_system_tests, deferred_job_execution) {
    asio::io_context main_ctx;
    job_system js(main_ctx);

    config_table cfg = config_table::load_from_source(std::format(kConfig, kNumWorkers));
    js.initialize(cfg);
    ASSERT_EQ(js.get_num_workers(), kNumWorkers);

    int counter = 0;

    auto job1 = js.submit(
      { .name = "Job 1" },
      [&counter]() {
        counter++;
      });
    ASSERT_NE(job1, nullptr);
    EXPECT_EQ(job1->get_status(), job::status::RUNNING);

    auto job2 = js.submit_deferred(
      job1->id,
      { .name = "Deferred Job", .priority = job::priority::HIGH, .thread_affinity = job::affinity::WORKER_THREAD },
      [&counter]() {
        counter++;
      });
    ASSERT_NE(job2, nullptr);
    EXPECT_EQ(job2->get_status(), job::status::PENDING);

    const size_t max_polls = 100000;
    size_t poll1 = 0;
    do {
      main_ctx.poll();
      js.poll();
      ++poll1;
    } while (!job1->done() && poll1 < max_polls);
    ASSERT_LT(poll1, max_polls);
    EXPECT_EQ(job1->get_status(), job::status::COMPLETED);
    EXPECT_EQ(job2->get_status(), job::status::RUNNING);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    EXPECT_GE(counter, 2);  // < this may not be 2 here because the second job is on a worker so it might have already been incrmented it is technically a race condition in the test yes, but get off my ass you sonofabitch you, leave me alone, im one man, a singular man, alone in this cruel world

    size_t poll2 = 0;
    do {
      main_ctx.poll();
      js.poll();
      ++poll2;
    } while (!job2->done() && poll2 < max_polls);
    ASSERT_LT(poll2, max_polls);
    EXPECT_EQ(job2->get_status(), job::status::COMPLETED);
    EXPECT_EQ(counter, 2);

    js.shutdown();
  }

}  // namespace other
/**
 * \file tests/core/logger_tests.cpp
 *
 * contract under test: logger error paths report failures and return instead of
 *  self-deadlocking on log_mutex (create_logger overflow, duplicate sink id).
 **/
#include "core/logger_tests.hpp"

namespace other {

  TEST_F(logger_tests, create_logger_overflow_fails_cleanly) {
    auto* log = subsystem<logger>::get();
    ASSERT_NE(log, nullptr);

    /// the fixture already created one logger; fill the table past capacity.
    /// before the deadlock fix this test would hang instead of failing
    bool saw_failure = false;
    for (int32_t i = 0; i < 300; ++i) {
      natural_t id = log->create_logger(std::format("overflow-logger-{}", i), spdlog::level::off);
      if (id == static_cast<natural_t>(-1)) {
        saw_failure = true;
        break;
      }
    }
    ASSERT_TRUE(saw_failure);
  }

  TEST_F(logger_tests, duplicate_sink_id_fails_cleanly) {
    auto* log = subsystem<logger>::get();
    ASSERT_NE(log, nullptr);

    /// sink id 1 is the fixture's console sink; re-registering must hit the
    ///  error path (which holds log_mutex) and return without deadlocking
    log_sink duplicate = {
      1, "duplicate-console-sink", "%v", spdlog::level::info,
      stdout_sink_fn,
    };
    std::string loggers[] = { "other-core-log" };
    log->register_sink(loggers, &duplicate);
    SUCCEED();
  }

}  // namespace other

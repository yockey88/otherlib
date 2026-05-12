/**
 * \file core/coroutine.cpp
 **/
#include "core/coroutine.hpp"

#include "core/logger.hpp"

namespace other {

  bool task::job_awaiter::await_ready() const noexcept {
    return !job_handle->running();
  }

  task::awaiter task::yield() {
    return awaiter{};
  }

  task task::sleep_for(asio::chrono::milliseconds duration) {
    auto left = duration.count();

    auto now = asio::chrono::steady_clock::now();
    auto last = now;

    while (left > 0) {
      co_await task::yield();
      now = asio::chrono::steady_clock::now();
      auto elapsed = asio::chrono::duration_cast<asio::chrono::milliseconds>(now - last).count();
      left -= elapsed;
      last = now;
    }
  }

  task task::wait_for_job(ref<job> job_handle) {
    OTHER_ASSERT(job_handle != nullptr, "Job handle is null in wait_for_job");
    co_await job_awaiter{ job_handle };
  }

}  // namespace other
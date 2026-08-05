/**
 * \file core/coroutine.hpp
 **/
#ifndef OTHER_CORE_CORE_COROUTINE_HPP
#define OTHER_CORE_CORE_COROUTINE_HPP

#include <chrono>
#include <coroutine>

#include "core/job.hpp"
#include "core/ref.hpp"

namespace other {

  struct null_yield {};

  using suspend_always = std::suspend_always;
  using suspend_never = std::suspend_never;

  /// C++ coroutines crash course
  /**
    coroutine<return-type> function_name(parameters...) {
      auto* frame = new coroutine_frame(std::forward<parameters...>(parameters));
      auto return_object = coroutine_frame->promise.get_return_object();
      co_await coroutine_frame->promise.initial_suspend();
      try
      {
          <body-statements>
      }
      catch (...)
      {
          coroutine_frame->promise.unhandled_exception();
      }
      co_await coroutine_frame->promise.final_suspend();
      delete coroutine_frame;
      return return_object;
    }
  */

  struct task {
    struct awaiter;
    struct final_awaiter;

    struct promise_type {
      task get_return_object() {
        return task{ std::coroutine_handle<promise_type>::from_promise(*this) };
      }

      void unhandled_exception() noexcept {}
      void return_void() noexcept {}

      awaiter initial_suspend() noexcept { return {}; }
      awaiter final_suspend() noexcept { return {}; }
    };

    /// identical to suspend_always
    struct awaiter {
      std::coroutine_handle<promise_type> handle;

      bool await_ready() const noexcept { return false; }
      void await_suspend(std::coroutine_handle<>) const noexcept {}
      void await_resume() const noexcept {}
    };

    struct job_awaiter {
      ref<job> job_handle;
      bool await_ready() const noexcept;
      void await_suspend(std::coroutine_handle<>) const noexcept {}
      void await_resume() const noexcept {}
    };

    std::coroutine_handle<promise_type> coro_handle;

    auto operator co_await() noexcept {
      return awaiter{ coro_handle };
    }

    void operator()() const {
      coro_handle.resume();
    }

    bool done() const {
      return coro_handle == nullptr || coro_handle.done();
    }

    /// helper tasks
    static task::awaiter yield();
    static task sleep_for(std::chrono::milliseconds duration);
    static task wait_for_job(ref<job> job_handle);
  };

}  // namespace other

#endif  // OTHER_CORE_CORE_COROUTINE_HPP
/**
 * \file core/coroutine.hpp
 **/
#ifndef OTHER_CORE_CORE_COROUTINE_HPP
#define OTHER_CORE_CORE_COROUTINE_HPP

#include <coroutine>

#include <asio/asio.hpp>

namespace other {

  struct null_yield {};

  using suspend_always = std::suspend_always;
  using suspend_never = std::suspend_never;

  struct task {
    struct promise_type;

    struct awaiter {
      /// initial suspend has to be suspend always so we can store and pump with the rest of the event loop
      /// this is the same as this returning false
      bool await_ready() const noexcept { return false; }
      void await_suspend(std::coroutine_handle<>) const noexcept {}
      void await_resume() const noexcept {}
    };

    // struct final_awaiter {
    //   bool await_ready() const noexcept { return false; }

    //   template <typename T>
    //   void await_suspend(std::coroutine_handle<T> handle) const noexcept {
    //     if (handle.promise().continuation) {
    //       handle.promise().continuation.resume();
    //     }
    //   }

    //   void await_resume() const noexcept {}
    // };

    struct promise_type {
      task get_return_object() {
        return task{ std::coroutine_handle<promise_type>::from_promise(*this) };
      }

      void unhandled_exception() noexcept {}

      void return_void() noexcept {}

      awaiter initial_suspend() noexcept { return {}; }
      /// we want to catch the final suspend to know when to remove the coroutine from the live list
      /// and also to do any continuation handling
      std::suspend_always final_suspend() noexcept { return {}; }
    };

    std::coroutine_handle<promise_type> coro_handle;

    auto operator co_await() noexcept {
      return awaiter{};
    }

    void operator()() const {
      coro_handle.resume();
    }

    /// helper tasks
    static task sleep_for(asio::chrono::milliseconds duration) {
      auto left = duration.count();

      auto now = asio::chrono::steady_clock::now();
      auto last = now;

      while (left > 0) {
        co_await task::awaiter{};
        now = asio::chrono::steady_clock::now();
        auto elapsed = asio::chrono::duration_cast<asio::chrono::milliseconds>(now - last).count();
        left -= elapsed;
        last = now;
      }
    }
  };

  struct worker {
    enum {
      WORKER_IDLE = 0,
      WORKER_BUSY,
    } state = WORKER_IDLE;

    template <typename Self>
    void operator()(Self& self) {
      switch (state) {
        case WORKER_IDLE: break;
        case WORKER_BUSY: break;
        default:
          break;
      }
    }
  };

}  // namespace other

#endif  // OTHER_CORE_CORE_COROUTINE_HPP
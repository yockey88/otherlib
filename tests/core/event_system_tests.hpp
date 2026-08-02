/**
 * \file tests/core/event_system_tests.hpp
 **/
#ifndef OTHER_TESTS_CORE_EVENT_SYSTEM_TESTS_HPP
#define OTHER_TESTS_CORE_EVENT_SYSTEM_TESTS_HPP

#include <chrono>
#include <thread>

#include "event/event_system.hpp"

#include "other_test.hpp"

namespace other {

  class event_system_tests : public other_test {
   protected:
    /// pump the io_context until the predicate holds or the deadline passes
    template <typename Predicate>
    static bool pump_until(asio::io_context& io, Predicate&& done, std::chrono::seconds timeout = std::chrono::seconds(5)) {
      auto deadline = std::chrono::steady_clock::now() + timeout;
      while (!done() && std::chrono::steady_clock::now() < deadline) {
        io.poll();
        if (io.stopped()) {
          io.restart();
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
      }
      return done();
    }
  };

}  // namespace other

#endif  // OTHER_TESTS_CORE_EVENT_SYSTEM_TESTS_HPP

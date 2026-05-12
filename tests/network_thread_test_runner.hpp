/**
 * \file network_thread_test_runner.hpp
 **/
#ifndef OTHER_NETWORK_NETWORK_THREAD_TEST_RUNNER_HPP
#define OTHER_NETWORK_NETWORK_THREAD_TEST_RUNNER_HPP

#include <gtest/gtest.h>

#include "core/time.hpp"

#include "network/network_thread.hpp"

#include "message/message_bus.hpp"

namespace other {

  struct network_thread_test_context {
    message_bus& bus;
    network_thread& net_thread;
  };

  using network_thread_test_body = std::function<bool(network_thread_test_context&)>;

  class network_thread_test_runner {
   public:
    network_thread_test_runner() = default;
    ~network_thread_test_runner() = default;

    ::testing::AssertionResult test_passes(const std::string_view test_name, network_thread_test_body test_body);

   private:
    microseconds default_timeout = microseconds(100);

    void perform_setup(network_thread_test_context& context);
    void perform_shutdown(network_thread_test_context& context);

    bool wait_on_message(const message_header& header, message_bus& bus);
    bool wait_on_acked_message(const message_header& original_header, message_bus& bus);
  };

}  // namespace other

#endif  // OTHER_NETWORK_NETWORK_THREAD_TEST_RUNNER_HPP
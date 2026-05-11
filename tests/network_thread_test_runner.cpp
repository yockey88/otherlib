/**
 * \file network_thread_test_runner.cpp
 **/
#include "network_thread_test_runner.hpp"

#include "core/enum_formatter.hpp"

#include "gtest/gtest.h"

namespace other {

  ::testing::AssertionResult network_thread_test_runner::test_passes(const std::string_view test_name, network_thread_test_body test_body) {
    CORE_LOG_INFO("Running network thread test '{}'", test_name);

    message_bus bus;
    network_thread thread{ bus };
    network_thread_test_context context{ bus, thread };
    perform_setup(context);

    bool success = false;
    try {
      EXPECT_TRUE(test_body(context));
      success = true;
    } catch (const std::exception& e) {
      CORE_LOG_ERROR("Exception occurred during test '{}': {}", test_name, e.what());
    } catch (...) {
      CORE_LOG_ERROR("Unknown exception occurred during test '{}'", test_name);
    }

    perform_shutdown(context);

    return success ?
      ::testing::AssertionSuccess() :
      ::testing::AssertionFailure() << std::format("[NET THREAD_TESTS: {}]: '{}'", "FAIL", test_name);
  }

  void network_thread_test_runner::perform_setup(network_thread_test_context& context) {
    ASSERT_NO_FATAL_FAILURE(context.net_thread.launch());
    ASSERT_TRUE(wait_on_message({ NOTIFICATION, NETWORK_THREAD_READY }, context.bus));
  }

  void network_thread_test_runner::perform_shutdown(network_thread_test_context& context) {
    message shutdown_msg{ COMMAND, SHUTDOWN_REQUEST, {} };
    message req_ack_msg{ REQUEST, ACK, {} };
    request_acknowledgment req_ack_data{
      .ack_id = 0,  // not used in this context
      .original_header = message_header{ shutdown_msg.category, shutdown_msg.id },
      .message_data = std::move(shutdown_msg.data),
    };
    ASSERT_NO_FATAL_FAILURE(req_ack_msg.data = serialize_direct(req_ack_data));
    ASSERT_NO_FATAL_FAILURE(context.bus.send_message(std::move(req_ack_msg)));

    ASSERT_TRUE(wait_on_acked_message({ shutdown_msg.category, shutdown_msg.id }, context.bus));
    ASSERT_NO_FATAL_FAILURE(context.net_thread.shutdown());
    ASSERT_NO_FATAL_FAILURE(context.net_thread.wait_for_shutdown_complete());
  }

  bool network_thread_test_runner::wait_on_message(const message_header& header, message_bus& bus) {
    bool found = false;

    const milliseconds duration(500);
    auto start_time = steady_clock::now();
    while (steady_clock::now() - start_time < duration) {
      opt<message> msg_opt = bus.receive_message(microseconds(10));
      if (msg_opt.has_value()) {
        auto& msg = *msg_opt;
        if (message_header{ msg.category, msg.id } == header) {
          found = true;
          break;
        }
      }
      std::this_thread::sleep_for(milliseconds(100));
    }

    return found;
  }

  bool network_thread_test_runner::wait_on_acked_message(const message_header& original_header, message_bus& bus) {
    bool found = false;

    const milliseconds duration(500);
    auto start_time = steady_clock::now();
    while (steady_clock::now() - start_time < duration) {
      opt<message> msg_opt = bus.receive_message(microseconds(10));
      if (msg_opt.has_value()) {
        auto& msg = *msg_opt;
        if (message_header{ msg.category, msg.id } == message_header{ ACKNOWLEDGEMENT, ACK }) {
          auto ack_data = deserialize_direct<acknowledgement_ack>(msg.data).first;
          if (ack_data.acked_header == original_header) {
            found = true;
            break;
          }
        }
      }
      std::this_thread::sleep_for(milliseconds(100));
    }

    return found;
  }

}  // namespace other
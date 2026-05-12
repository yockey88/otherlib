/**
 * \file network_thread_tests.cpp
 **/
#include "network_thread_tests.hpp"

#include <gtest/gtest.h>

#include <mswsockdef.h>


namespace other {

  bool network_thread_tests::wait_on_message(const message_header& header, message_bus& bus, microseconds timeout) {
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

  bool network_thread_tests::wait_on_acked_message(const message_header& original_header, message_bus& bus, microseconds timeout) {
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

  TEST_F(network_thread_tests, basic_verification) {
    message_bus bus;
    network_thread mock_thread{ bus };

    ASSERT_NO_FATAL_FAILURE(mock_thread.launch());

    const milliseconds duration(500);
    const auto start_time = steady_clock::now();
    while (steady_clock::now() - start_time < duration) {
      std::this_thread::sleep_for(milliseconds(100));
    }

    ASSERT_NO_FATAL_FAILURE(mock_thread.shutdown());
    ASSERT_NO_FATAL_FAILURE(mock_thread.wait_for_shutdown_complete());
  }

  TEST_F(network_thread_tests, verify_message_bus_communication) {
    message_bus bus;
    network_thread mock_thread{ bus };

    ASSERT_FALSE(bus.has_message());
    ASSERT_NO_FATAL_FAILURE(mock_thread.launch());

    ASSERT_TRUE(wait_on_message({ NOTIFICATION, NETWORK_THREAD_READY }, bus));

    {
      message shutdown_msg{ COMMAND, SHUTDOWN_REQUEST, {} };
      message req_ack_msg{ REQUEST, ACK, {} };
      request_acknowledgment req_ack_data{
        .ack_id = 1,
        .original_header = { shutdown_msg.category, shutdown_msg.id },
        .message_data = std::move(shutdown_msg.data),
      };
      ASSERT_NO_FATAL_FAILURE(req_ack_msg.data = serialize_direct(req_ack_data));
      ASSERT_NO_FATAL_FAILURE(bus.send_message(std::move(req_ack_msg)));
    }

    ASSERT_TRUE(wait_on_acked_message({ COMMAND, SHUTDOWN_REQUEST }, bus));

    ASSERT_NO_FATAL_FAILURE(mock_thread.shutdown());
    ASSERT_NO_FATAL_FAILURE(mock_thread.wait_for_shutdown_complete());

    ASSERT_TRUE(bus.has_message());
    {
      opt<message> msg_opt = bus.receive_message();
      ASSERT_TRUE(msg_opt.has_value());

      message msg = std::move(*msg_opt);
      EXPECT_EQ(msg.category, NOTIFICATION);
      EXPECT_EQ(msg.id, NETWORK_THREAD_SHUTDOWN_COMPLETE);
    }
  }

}  // namespace other
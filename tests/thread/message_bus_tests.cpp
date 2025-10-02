/**
 * \file thread/message_bus_tests.cpp
 **/
#include <barrier>
#include <chrono>
#include <ratio>

#include "thread/message_bus.hpp"

#include "thread_tests.hpp"

namespace other {

  TEST_F(thread_tests, message_bus_basic_send_receive) {
    std::barrier sync_point(3);

    message_bus bus;

    std::thread t1 = std::thread{ [&]() {
      bus.register_thread();
      sync_point.arrive_and_wait();

      opt<message> msg = bus.receive_message(std::chrono::milliseconds(500));
      ASSERT_TRUE(msg.has_value());

      ASSERT_EQ(msg->header.category, 0xBEEF);
      ASSERT_EQ(msg->header.id, 0xDEAD);
      ASSERT_EQ(msg->data.size(), 3);
      ASSERT_EQ(msg->data[0], 0x01);
      ASSERT_EQ(msg->data[1], 0x02);
      ASSERT_EQ(msg->data[2], 0x03);

      message resp_msg = {};
      resp_msg.header = message_header{ .category = 0xCAFE, .id = 0xFACE };
      resp_msg.data = { 0x04, 0x05, 0x06 };
      bus.send_message(std::move(resp_msg));
    } };

    std::thread t2 = std::thread{ [&]() {
      bus.register_thread();
      sync_point.arrive_and_wait();

      message msg = {};
      msg.header = message_header{ .category = 0xBEEF, .id = 0xDEAD };
      msg.data = { 0x01, 0x02, 0x03 };
      bus.send_message(std::move(msg));

      opt<message> recv_msg = bus.receive_message(std::chrono::milliseconds(500));
      ASSERT_TRUE(recv_msg.has_value());
      ASSERT_EQ(recv_msg->header.category, 0xCAFE);
      ASSERT_EQ(recv_msg->header.id, 0xFACE);

      ASSERT_EQ(recv_msg->data.size(), 3);
      ASSERT_EQ(recv_msg->data[0], 0x04);
      ASSERT_EQ(recv_msg->data[1], 0x05);
      ASSERT_EQ(recv_msg->data[2], 0x06);
    } };

    sync_point.arrive_and_wait();

    ASSERT_TRUE(t1.joinable());
    t1.join();

    ASSERT_TRUE(t2.joinable());
    t2.join();
  }

  TEST_F(thread_tests, message_bus_failure_to_register) {
    message_bus bus;

    ASSERT_DEATH(
      {
        std::thread t1 = std::thread{ [&]() {
          bus.register_thread();
          bus.register_thread();  // second registration should fail
        } };
      },
      ""
    );

    ASSERT_DEATH(
      {
        std::thread t2 = std::thread{ [&]() {
          bus.receive_message();  // not registered yet
        } };
      },
      ""
    );

    ASSERT_DEATH(
      {
        std::thread t2 = std::thread{ [&]() {
          message send_msg = {};
          send_msg.header = message_header{ .category = 0xDEAD };
          bus.send_message(std::move(send_msg));  // not registered yet
        } };
      },
      ""
    );
  }

}  // namespace other
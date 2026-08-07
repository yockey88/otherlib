/**
 * \file message/message_bus_tests.cpp
 **/
#include <barrier>
#include <chrono>
#include <ratio>

#include "message/message_bus.hpp"

#include "../thread/thread_tests.hpp"

namespace other {

  TEST_F(thread_tests, message_bus_basic_send_receive) {
    std::barrier sync_point(3);

    message_bus bus;

    std::thread t1 = std::thread{ [&]() {
      bus.register_thread();
      sync_point.arrive_and_wait();

      opt<message> msg = bus.receive_message(std::chrono::milliseconds(500));
      ASSERT_TRUE(msg.has_value());

      ASSERT_EQ(msg->category, 0xBEEF);
      ASSERT_EQ(msg->id, 0xDEAD);
      ASSERT_EQ(msg->data.size(), 3);
      ASSERT_EQ(msg->data[0], 0x01);
      ASSERT_EQ(msg->data[1], 0x02);
      ASSERT_EQ(msg->data[2], 0x03);

      message resp_msg = {};
      resp_msg.category = 0xCAFE;
      resp_msg.id = 0xFACE;
      resp_msg.data = { 0x04, 0x05, 0x06 };
      bus.send_message(std::move(resp_msg));
    } };

    std::thread t2 = std::thread{ [&]() {
      bus.register_thread();
      sync_point.arrive_and_wait();

      message msg = {};
      msg.category = 0xBEEF;
      msg.id = 0xDEAD;
      msg.data = { 0x01, 0x02, 0x03 };
      bus.send_message(std::move(msg));

      opt<message> recv_msg = bus.receive_message(std::chrono::milliseconds(500));
      ASSERT_TRUE(recv_msg.has_value());
      ASSERT_EQ(recv_msg->category, 0xCAFE);
      ASSERT_EQ(recv_msg->id, 0xFACE);

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
          send_msg.category = 0xDEAD;
          bus.send_message(std::move(send_msg));  // not registered yet
        } };
      },
      ""
    );
  }

  TEST_F(thread_tests, channel_await_without_timeout_blocks_until_push) {
    ref<channel_queue<message>> queue = make_ref<channel_queue<message>>();
    auto [producer, consumer] = channel<message>::make_channel(queue);

    /// no-timeout await must block on an empty queue (popping blind was UB), then wake
    std::thread pusher{ [&]() {
      std::this_thread::sleep_for(std::chrono::milliseconds(50));
      message msg = {};
      msg.category = 0xBEEF;
      msg.id = 0xF00D;
      producer->push(std::move(msg));
    } };

    const auto start = std::chrono::steady_clock::now();
    opt<message> received = consumer->await_message();
    const auto waited = std::chrono::steady_clock::now() - start;

    pusher.join();

    ASSERT_TRUE(received.has_value());
    EXPECT_EQ(received->category, 0xBEEF);
    EXPECT_EQ(received->id, 0xF00D);
    EXPECT_GE(waited, std::chrono::milliseconds(30));
  }

}  // namespace other
/**
 * \file data-structures/spsc_buffer_tests.cpp
 **/
#include "data-structures/spsc_buffer_tests.hpp"

#include "data-structures/spsc_buffer.hpp"

namespace other {

  TEST_F(spsc_buffer_tests, push_and_pop) {
    spsc_buffer<int, 4> buffer;

    ASSERT_TRUE(buffer.empty());
    ASSERT_EQ(buffer.size(), 0);
    ASSERT_EQ(buffer.capacity(), 4);

    buffer.push(1);
    ASSERT_EQ(buffer.size(), 1);

    buffer.push(2);
    ASSERT_EQ(buffer.size(), 2);

    buffer.push(3);
    EXPECT_EQ(buffer.size(), 3);

    auto item1 = buffer.pop();
    EXPECT_TRUE(item1.has_value());
    EXPECT_EQ(item1.value(), 1);
    ASSERT_EQ(buffer.size(), 2);

    auto item2 = buffer.pop();
    EXPECT_TRUE(item2.has_value());
    EXPECT_EQ(item2.value(), 2);
    ASSERT_EQ(buffer.size(), 1);

    auto item3 = buffer.pop();
    EXPECT_TRUE(item3.has_value());
    EXPECT_EQ(item3.value(), 3);
    ASSERT_EQ(buffer.size(), 0);
  }

  TEST_F(spsc_buffer_tests, overwrite_behavior) {
    spsc_buffer<int, 2> buffer;

    buffer.push(1);
    ASSERT_EQ(buffer.size(), 1);

    buffer.push(2);
    ASSERT_EQ(buffer.size(), 2);
    ASSERT_EQ(buffer.size(), buffer.capacity());

    // This push should overwrite the oldest item (1)
    buffer.push(3);
    ASSERT_EQ(buffer.size(), 2);
    ASSERT_EQ(buffer.size(), buffer.capacity());

    auto item1 = buffer.pop();
    EXPECT_TRUE(item1.has_value());
    EXPECT_EQ(item1.value(), 2);

    auto item2 = buffer.pop();
    EXPECT_TRUE(item2.has_value());
    EXPECT_EQ(item2.value(), 3);
  }

  TEST_F(spsc_buffer_tests, empty_pop) {
    spsc_buffer<int, 2> buffer;

    auto item = buffer.pop();
    ASSERT_FALSE(item.has_value());
  }

  TEST_F(spsc_buffer_tests, byte_packets) {
    spsc_buffer<std::vector<uint8_t>, 10> buffer;

    {
      std::vector<uint8_t> packet1 = { 0x01, 0x02, 0x03 };
      std::vector<uint8_t> packet2 = { 0x0A, 0x0B, 0x0C, 0x0D };

      buffer.push(std::move(packet1));
      buffer.push(std::move(packet2));
    }
    ASSERT_EQ(buffer.size(), 2);

    {
      auto popped_packet1 = buffer.pop();
      ASSERT_TRUE(popped_packet1.has_value());
      EXPECT_EQ(popped_packet1.value()[0], 0x01);
      EXPECT_EQ(popped_packet1.value()[1], 0x02);
      EXPECT_EQ(popped_packet1.value()[2], 0x03);
    }

    {
      std::vector<uint8_t> packet3 = { 0xFF, 0xEE };
      buffer.push(std::move(packet3));
    }

    {
      auto popped_packet2 = buffer.pop();
      ASSERT_TRUE(popped_packet2.has_value());
      EXPECT_EQ(popped_packet2.value()[0], 0x0A);
      EXPECT_EQ(popped_packet2.value()[1], 0x0B);
      EXPECT_EQ(popped_packet2.value()[2], 0x0C);
      EXPECT_EQ(popped_packet2.value()[3], 0x0D);
    }

    {
      auto popped_packet3 = buffer.pop();
      ASSERT_TRUE(popped_packet3.has_value());
      EXPECT_EQ(popped_packet3.value()[0], 0xFF);
      EXPECT_EQ(popped_packet3.value()[1], 0xEE);
    }
  }

  TEST_F(spsc_buffer_tests, thread_safety) {
    spsc_buffer<int, 1000> buffer;

    std::thread producer([&buffer]() {
      for (int i = 0; i < 1000; ++i) {
        buffer.push(i);
      }
    });

    std::thread consumer([&buffer]() {
      int expected_value = 0;
      while (expected_value < 1000) {
        if (buffer.empty()) {
          std::this_thread::yield();
          continue;
        }

        auto item = buffer.pop();
        if (item.has_value()) {
          EXPECT_EQ(item.value(), expected_value);
          ++expected_value;
        }
      }
    });

    producer.join();
    consumer.join();
  }

  TEST_F(spsc_buffer_tests, thread_safety_2) {
    using packet_t = std::vector<uint8_t>;
    spsc_buffer<packet_t, 1000> buffer;

    std::thread producer([&buffer]() {
      for (int i = 0; i < 1000; ++i) {
        packet_t p = { static_cast<uint8_t>(i & 0xFF) };
        buffer.push(std::move(p));
      }
    });

    std::thread consumer([&buffer]() {
      int expected_value = 0;
      while (expected_value < 1000) {
        if (buffer.empty()) {
          std::this_thread::yield();
          continue;
        }

        auto item = buffer.pop();
        if (item.has_value()) {
          EXPECT_EQ(item.value()[0], static_cast<uint8_t>(expected_value & 0xFF));
          ++expected_value;
        }
      }
    });

    producer.join();
    consumer.join();
  }

}  // namespace other
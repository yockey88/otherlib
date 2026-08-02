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
    spsc_buffer<std::vector<uint8_t>, 16> buffer;

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

  namespace {

    /// records the ids of destroyed elements; moved-from probes go silent (id = -1)
    struct overwrite_probe {
      int32_t id = -1;
      inline static std::vector<int32_t> destroyed_ids{};

      overwrite_probe() = default;
      explicit overwrite_probe(int32_t i) : id(i) {}
      overwrite_probe(overwrite_probe&& other) noexcept : id(other.id) { other.id = -1; }
      ~overwrite_probe() {
        if (id >= 0) {
          destroyed_ids.push_back(id);
        }
      }
    };

  }  // namespace

  TEST_F(spsc_buffer_tests, wraparound_preserves_fifo_order) {
    spsc_buffer<int, 4> buffer;

    /// indices grow monotonically; 100 items through a 4-slot ring crosses the
    ///  capacity boundary 25 times
    for (int i = 0; i < 100; ++i) {
      buffer.push(i);
      auto item = buffer.pop();
      ASSERT_TRUE(item.has_value());
      ASSERT_EQ(item.value(), i);
    }
    ASSERT_TRUE(buffer.empty());
  }

  TEST_F(spsc_buffer_tests, wraparound_with_partial_drains) {
    spsc_buffer<int, 4> buffer;

    int next_push = 0;
    int next_pop = 0;
    for (int round = 0; round < 50; ++round) {
      buffer.push(next_push++);
      buffer.push(next_push++);
      buffer.push(next_push++);
      for (int i = 0; i < 3; ++i) {
        auto item = buffer.pop();
        ASSERT_TRUE(item.has_value());
        ASSERT_EQ(item.value(), next_pop++);
      }
    }
    ASSERT_TRUE(buffer.empty());
  }

  TEST_F(spsc_buffer_tests, overwrite_destroys_oldest_element) {
    overwrite_probe::destroyed_ids.clear();
    {
      spsc_buffer<overwrite_probe, 2> buffer;

      buffer.push(overwrite_probe{ 1 });
      buffer.push(overwrite_probe{ 2 });
      ASSERT_TRUE(overwrite_probe::destroyed_ids.empty());

      /// full: pushing 3 must destroy the oldest live element (1), not leak it
      buffer.push(overwrite_probe{ 3 });
      ASSERT_EQ(overwrite_probe::destroyed_ids, (std::vector<int32_t>{ 1 }));

      auto first = buffer.pop();
      ASSERT_TRUE(first.has_value());
      ASSERT_EQ(first.value().id, 2);

      auto second = buffer.pop();
      ASSERT_TRUE(second.has_value());
      ASSERT_EQ(second.value().id, 3);
    }
  }

  TEST_F(spsc_buffer_tests, threaded_wraparound_bounded_producer) {
    constexpr int kItemCount = 10000;
    spsc_buffer<int, 64> buffer;

    std::thread producer([&buffer]() {
      for (int i = 0; i < kItemCount; ++i) {
        /// wait for space so nothing is overwritten; the ring wraps ~150 times
        while (buffer.size() == buffer.capacity()) {
          std::this_thread::yield();
        }
        buffer.push(i);
      }
    });

    std::thread consumer([&buffer]() {
      int expected_value = 0;
      while (expected_value < kItemCount) {
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
    ASSERT_TRUE(buffer.empty());
  }

  TEST_F(spsc_buffer_tests, thread_safety) {
    spsc_buffer<int, 1024> buffer;

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
    spsc_buffer<packet_t, 1024> buffer;

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
/**
 * \file network/slot_registry_tests.cpp
 **/
#include <atomic>
#include <thread>

#include <gtest/gtest.h>

#include "thread/slot_registry.hpp"

#include "other_test.hpp"

namespace other {

  class slot_registry_tests : public other_test {};

  namespace {

    constexpr uint64_t kSentinel = 0x53454E54494E454C;

    struct probe_entry {
      uint64_t magic = kSentinel;
      uint64_t value = 0;
    };

  }  // namespace

  TEST_F(slot_registry_tests, insert_publish_iterate) {
    slot_registry<probe_entry, 4> registry;
    probe_entry a{ .value = 1 }, b{ .value = 2 }, c{ .value = 3 };

    EXPECT_TRUE(registry.insert(&a));
    EXPECT_TRUE(registry.insert(&b));
    EXPECT_TRUE(registry.insert(&c));
    EXPECT_EQ(registry.count(), 3u);
    EXPECT_TRUE(registry.contains(&b));

    uint64_t sum = 0;
    registry.for_each([&](probe_entry& e) { sum += e.value; });
    EXPECT_EQ(sum, 6u);

    probe_entry* found = registry.find_if([](const probe_entry& e) { return e.value == 2; });
    EXPECT_EQ(found, &b);
  }

  TEST_F(slot_registry_tests, tombstone_skipped_and_slot_reused) {
    slot_registry<probe_entry, 4> registry;
    probe_entry a{ .value = 1 }, b{ .value = 2 }, c{ .value = 3 }, d{ .value = 4 };

    ASSERT_TRUE(registry.insert(&a));
    ASSERT_TRUE(registry.insert(&b));
    ASSERT_TRUE(registry.insert(&c));

    EXPECT_TRUE(registry.erase(&b));
    EXPECT_FALSE(registry.contains(&b));
    EXPECT_EQ(registry.count(), 2u);

    uint64_t sum = 0;
    registry.for_each([&](probe_entry& e) { sum += e.value; });
    EXPECT_EQ(sum, 4u);

    /// the tombstoned slot is reused, so a fourth live entry still fits alongside capacity
    EXPECT_TRUE(registry.insert(&d));
    EXPECT_EQ(registry.count(), 3u);
    EXPECT_TRUE(registry.contains(&d));
  }

  TEST_F(slot_registry_tests, rejects_full_null_and_duplicate) {
    slot_registry<probe_entry, 2> registry;
    probe_entry a, b, c;

    EXPECT_FALSE(registry.insert(nullptr));
    EXPECT_TRUE(registry.insert(&a));
    EXPECT_FALSE(registry.insert(&a));
    EXPECT_TRUE(registry.insert(&b));
    EXPECT_FALSE(registry.insert(&c));

    EXPECT_TRUE(registry.erase(&a));
    EXPECT_FALSE(registry.erase(&a));
    EXPECT_TRUE(registry.insert(&c));
  }

  TEST_F(slot_registry_tests, reset_tombstones_everything) {
    slot_registry<probe_entry, 4> registry;
    probe_entry a, b;
    ASSERT_TRUE(registry.insert(&a));
    ASSERT_TRUE(registry.insert(&b));

    registry.reset();
    EXPECT_EQ(registry.count(), 0u);
    EXPECT_FALSE(registry.contains(&a));

    EXPECT_TRUE(registry.insert(&a));
    EXPECT_EQ(registry.count(), 1u);
  }

  TEST_F(slot_registry_tests, insert_becomes_visible_to_reader_thread) {
    slot_registry<probe_entry, 4> registry;
    probe_entry a{ .value = 7 };

    std::atomic<bool> seen = false;
    std::jthread reader([&](std::stop_token stoken) {
      while (!stoken.stop_requested() && !seen.load(std::memory_order_relaxed)) {
        registry.for_each([&](probe_entry& e) {
          if (e.value == 7) {
            seen.store(true, std::memory_order_relaxed);
          }
        });
      }
    });

    std::this_thread::sleep_for(milliseconds(10));
    ASSERT_TRUE(registry.insert(&a));

    const auto deadline = steady_clock::now() + seconds(2);
    while (!seen.load(std::memory_order_relaxed) && steady_clock::now() < deadline) {
      std::this_thread::yield();
    }
    EXPECT_TRUE(seen.load(std::memory_order_relaxed));
  }

  TEST_F(slot_registry_tests, reader_never_observes_torn_entries_under_churn) {
    slot_registry<probe_entry, 8> registry;
    probe_entry entries[6];
    for (auto& e : entries) {
      e.magic = kSentinel;
    }

    std::atomic<bool> corrupt = false;
    std::atomic<uint64_t> reads = 0;
    std::jthread reader([&](std::stop_token stoken) {
      while (!stoken.stop_requested()) {
        registry.for_each([&](probe_entry& e) {
          reads.fetch_add(1, std::memory_order_relaxed);
          if (e.magic != kSentinel) {
            corrupt.store(true, std::memory_order_relaxed);
          }
        });
      }
    });

    /// single-writer churn: register/unregister waves while the reader iterates freely
    for (int wave = 0; wave < 5000; ++wave) {
      for (auto& e : entries) {
        ASSERT_TRUE(registry.insert(&e));
      }
      for (auto& e : entries) {
        ASSERT_TRUE(registry.erase(&e));
      }
      if (corrupt.load(std::memory_order_relaxed)) {
        break;
      }
    }

    /// churn can outrun the reader's first pass (release builds finish inside one scheduling
    ///  quantum), so park one live entry and wait for the reader to prove it ran
    ASSERT_TRUE(registry.insert(&entries[0]));
    const auto deadline = steady_clock::now() + seconds(2);
    while (reads.load(std::memory_order_relaxed) == 0 && steady_clock::now() < deadline) {
      std::this_thread::yield();
    }

    reader.request_stop();
    reader.join();

    EXPECT_FALSE(corrupt.load(std::memory_order_relaxed));
    EXPECT_GT(reads.load(std::memory_order_relaxed), 0u);
  }

}  // namespace other

/**
 * \file data-structures/memory_pool_tests.cpp
 **/
#include "data-structures/memory_pool_tests.hpp"

namespace other {

  TEST_F(memory_pool_tests, move_constructor) {
    memory_pool<int, 10> pool1(true);
    auto [obj1, idx1] = pool1.emplace();
    obj1 = 42;

    memory_pool<int, 10> pool2(std::move(pool1));

    EXPECT_EQ(pool2.object_count(), 1);
    EXPECT_EQ(pool2[idx1], 42);
    EXPECT_EQ(pool1.object_count(), 0);
  }

  TEST_F(memory_pool_tests, free_runs_destructor_and_updates_counts) {
    pool_probe::reset();
    memory_pool<pool_probe, 4> pool;

    auto [obj, idx] = pool.emplace();
    ASSERT_EQ(pool_probe::constructed, 1);
    ASSERT_EQ(pool.size(), 1);
    ASSERT_FALSE(pool.empty());

    pool.free(idx);
    ASSERT_EQ(pool_probe::destroyed, 1);
    ASSERT_EQ(pool.size(), 0);
    ASSERT_TRUE(pool.empty());

    /// double free is a no-op, not an abort and not a second destruction
    pool.free(idx);
    ASSERT_EQ(pool_probe::destroyed, 1);
    ASSERT_EQ(pool.size(), 0);
  }

  TEST_F(memory_pool_tests, full_and_empty_track_live_objects) {
    memory_pool<int, 4> pool;
    std::array<size_t, 4> idxs{};

    for (size_t i = 0; i < 4; ++i) {
      auto [obj, idx] = pool.emplace();
      obj = static_cast<int>(i);
      idxs[i] = idx;
    }
    ASSERT_TRUE(pool.full());
    ASSERT_EQ(pool.size(), 4);
    ASSERT_EQ(pool.free_objects(), 0);

    pool.free(idxs[2]);
    ASSERT_FALSE(pool.full());
    ASSERT_EQ(pool.size(), 3);
    ASSERT_EQ(pool.free_objects(), 1);

    for (size_t i = 0; i < 4; ++i) {
      if (i != 2) {
        pool.free(idxs[i]);
      }
    }
    ASSERT_TRUE(pool.empty());
    ASSERT_EQ(pool.free_objects(), 4);
  }

  TEST_F(memory_pool_tests, churn_beyond_lifetime_capacity) {
    memory_pool<int, 4> pool;
    std::array<size_t, 4> idxs{};

    for (size_t i = 0; i < 4; ++i) {
      auto [obj, idx] = pool.emplace();
      idxs[i] = idx;
    }
    ASSERT_TRUE(pool.full());

    /// a pool must survive far more than Max lifetime allocations as long as
    ///  the live count stays within capacity
    for (int32_t round = 0; round < 100; ++round) {
      pool.free(idxs[0]);
      ASSERT_FALSE(pool.full());
      ASSERT_EQ(pool.size(), 3);

      auto [obj, idx] = pool.emplace();
      ASSERT_EQ(idx, idxs[0]);
      ASSERT_TRUE(pool.full());
    }
    ASSERT_EQ(pool.size(), 4);
  }

  TEST_F(memory_pool_tests, clear_resets_pool) {
    pool_probe::reset();
    memory_pool<pool_probe, 4> pool;

    pool.emplace();
    pool.emplace();
    ASSERT_EQ(pool.size(), 2);

    pool.clear();
    ASSERT_TRUE(pool.empty());
    ASSERT_FALSE(pool.full());
    ASSERT_EQ(pool_probe::destroyed, 2);

    auto [obj, idx] = pool.emplace();
    ASSERT_EQ(idx, 0);
    ASSERT_EQ(pool.size(), 1);
  }

}  // namespace other

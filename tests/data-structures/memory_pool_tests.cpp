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

}  // namespace other
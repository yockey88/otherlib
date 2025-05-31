/**
 * \file tests/arena/arena_test.hpp
 * Test suite for the arena memory allocator class
 */
#ifndef OTHER_TESTS_ARENA_TEST_HPP
#define OTHER_TESTS_ARENA_TEST_HPP

#include <cstdint>
#include <vector>

#include <gtest/gtest.h>

#include "core/arena.hpp"

namespace other {

  class arena_test : public ::testing::Test {
   protected:
    static constexpr size_t kTestBlockSize = 64;
    static constexpr size_t kLargeBlockSize = 1024;
    static constexpr size_t kMaxTestAllocations = 1000;
    static constexpr size_t kPageSize = arena_storage::kPageSize;
    static constexpr size_t kAlignment = arena_storage::kAlignment;

    struct test_allocation_data {
      void* ptr;
      size_t size;
      uint64_t pattern;
    };

    void SetUp() override {
      subsystem<arena>::get();
    }

    void TearDown() override {
      allocations.clear();
      subsystem<arena>::shutdown();
    }

    std::vector<test_allocation_data> allocations;

    void print_current_page();

    void verify_alignment(void* ptr, size_t alignment = kAlignment);
    void test_memory_boundaries(void* ptr, size_t size);

    void* allocate_and_verify(size_t size);

    void test_allocation_pattern(const std::vector<size_t>& sizes);
    void simulate_memory_pressure();
    void verify_arena_state();
  };

}  // namespace other

#endif  // OTHER_TESTS_ARENA_TEST_HPP

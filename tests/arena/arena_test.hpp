/**
 * \file tests/arena/arena_test.hpp
 */
#ifndef OTHER_TESTS_ARENA_TEST_HPP
#define OTHER_TESTS_ARENA_TEST_HPP

#include <cstdint>
#include <vector>

#include "memory/arena.hpp"

#include "other_test.hpp"

namespace other {

  class arena_test : public other_test {
   protected:
    static constexpr size_t kTestBlockSize = 64;
    static constexpr size_t kLargeBlockSize = 1024;
    static constexpr size_t kMaxTestAllocations = 1000;
    static constexpr size_t kPageSize = page::kPageSize;
    static constexpr size_t kAlignment = page::kAlignment;

    struct test_allocation_data {
      void* ptr;
      size_t size;
      uint64_t pattern;
    };

    std::vector<test_allocation_data> allocations;

    void print_current_page();

    void verify_alignment(void* ptr, size_t alignment = kAlignment);
    void verify_allocation_header(void* ptr, size_t size);
    void test_memory_boundaries(void* ptr, size_t size);

    void* allocate_and_verify(size_t size);

    void test_allocation_pattern(const std::vector<size_t>& sizes);
    void simulate_memory_pressure();
    void verify_arena_state();

    void TearDown() override {
      other_test::TearDown();
      allocations.clear();
    }
  };

}  // namespace other

#endif  // OTHER_TESTS_ARENA_TEST_HPP

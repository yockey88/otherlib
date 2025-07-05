/**
 * \file tests/arena/arena_buffer_test.cpp
 **/
#include "arena_buffer_test.hpp"

#include "core/arena.hpp"

#include "gtest/gtest.h"

namespace other {

  void arena_buffer_test::load_buffer_with_matrices(arena_buffer& buffer, size_t num_matrices) {
    glm::mat4 transform = glm::mat4(1.f);
    for (size_t i = 0; i < num_matrices; ++i) {
      buffer.buffer_data(transform);
    }
  }

  TEST_F(arena_buffer_test, safe_double_release) {
    int32_t val = 333;
    arena_buffer b;
    EXPECT_NO_FATAL_FAILURE(b.buffer_data(val));

    ASSERT_EQ(b.size(), sizeof(int32_t));
    ASSERT_EQ(b.num_elements(), 1);
    ASSERT_EQ(b.at<int32_t>(0), val);

    EXPECT_NO_FATAL_FAILURE(b.release());
    EXPECT_NO_FATAL_FAILURE(b.release());
  }

  TEST_F(arena_buffer_test, sequential_allocations_w_initial_resize) {
    int32_t value = 33u;
    arena_buffer buffer;

    /// capacity is 0 here so we expect an initial allocation of sizeof(int32_t) * 64
    EXPECT_NO_FATAL_FAILURE(buffer.buffer_data(value));
    ASSERT_EQ(buffer.max_size(), sizeof(int32_t) * 64);
    ASSERT_EQ(buffer.size(), sizeof(int32_t));
    ASSERT_EQ(buffer.num_elements(), 1);
    EXPECT_EQ(buffer.at<int32_t>(0), value);

    EXPECT_NO_FATAL_FAILURE(buffer.buffer_data(value * 2));
    EXPECT_EQ(buffer.max_size(), sizeof(int32_t) * 64);
    EXPECT_EQ(buffer.size(), sizeof(int32_t) * 2);
    EXPECT_EQ(buffer.at<int32_t>(0), value);
    EXPECT_EQ(buffer.at<int32_t>(1), value * 2);
    EXPECT_EQ(buffer.num_elements(), 2);

    EXPECT_NO_FATAL_FAILURE(buffer.buffer_data(value * 3));
    EXPECT_EQ(buffer.max_size(), sizeof(int32_t) * 64);
    EXPECT_EQ(buffer.size(), sizeof(int32_t) * 3);
    EXPECT_EQ(buffer.at<int32_t>(0), value);
    EXPECT_EQ(buffer.at<int32_t>(1), value * 2);
    EXPECT_EQ(buffer.at<int32_t>(2), value * 3);
    EXPECT_EQ(buffer.num_elements(), 3);

    EXPECT_NO_FATAL_FAILURE(buffer.release());
  }

  TEST_F(arena_buffer_test, array_test) {
    std::array<size_t, 10> arr = {
      1, 2, 3, 4, 5, 6, 7, 8, 9, 10
    };

    {
      arena_buffer buffer{ arr.data(), sizeof(size_t) * arr.size() };
      EXPECT_EQ(buffer.size(), sizeof(size_t) * arr.size());
    }

    arena_buffer buffer;
    buffer.write_arr(arr);
    EXPECT_EQ(buffer.size(), sizeof(size_t) * arr.size());
    EXPECT_EQ(buffer.num_elements(), arr.size());

    for (uint32_t i = 0; i < 10; ++i) {
      EXPECT_EQ(buffer.at<size_t>(i), arr[i]) << "Failed on .At<> test on step " << i;
    }

    /// write one more because we are at capacity so now we should extend
    size_t val = 11;
    EXPECT_NO_FATAL_FAILURE(buffer.buffer_data(val));
    // EXPECT_EQ(buffer.size(), sizeof(size_t) * (arr.size() + 1));
    // EXPECT_EQ(buffer.num_elements(), arr.size() + 1);
    // EXPECT_EQ(buffer.at<size_t>(10), val) << "Failed on .At<> test on step 10";
  }

  TEST_F(arena_buffer_test, idx_from_buffer_data) {
    uint32_t val = 33u;
    uint64_t val2 = 44u;

    arena_buffer buffer;

    glm::mat4 mat4 = glm::mat4(1.f);
    glm::vec2 vec2 = glm::vec2(1.f);

    size_t val_idx;
    size_t val2_idx;
    size_t mat4_idx;
    size_t vec2_idx;

    EXPECT_NO_FATAL_FAILURE(val_idx = buffer.buffer_data(val));
    EXPECT_NO_FATAL_FAILURE(val2_idx = buffer.buffer_data(val2));
    EXPECT_NO_FATAL_FAILURE(mat4_idx = buffer.buffer_data(mat4));
    EXPECT_NO_FATAL_FAILURE(vec2_idx = buffer.buffer_data(vec2));

    EXPECT_EQ(buffer.at<uint32_t>(val_idx), val);
    EXPECT_EQ(buffer.at<uint64_t>(val2_idx), val2);
    EXPECT_EQ(buffer.at<glm::mat4>(mat4_idx), mat4);
    EXPECT_EQ(buffer.at<glm::vec2>(vec2_idx), vec2);

    ASSERT_EQ(val_idx, 0u);
    ASSERT_EQ(val2_idx, 1u);
    ASSERT_EQ(mat4_idx, 2u);
    ASSERT_EQ(vec2_idx, 3u);
  }

  /// tests writing glm types to copy into uniform on draw
  TEST_F(arena_buffer_test, death_uniform_buffer_compatability) {
    using namespace std::string_view_literals;

    static_assert(sizeof(glm::mat4) == 4 * 4 * sizeof(float), "mat4 type has unexpected size!");
    static_assert(sizeof(glm::vec4) == 4 * sizeof(float), "vec4 type has unexpeccted size!");

    arena_buffer buffer;
    {
      glm::mat4 transform = glm::mat4(1.f);
      glm::vec4 vector = glm::vec4(1.f);

      /// after first write:
      ///   capacity == sizeof(glm::mat4) * 64 == 64 * 64 == 4096
      ///   size == 64
      /// then we shouldn't have to resize again in this test there should be no more resizes
      EXPECT_NO_FATAL_FAILURE(buffer.buffer_data(transform));
      EXPECT_EQ(buffer.max_size(), sizeof(glm::mat4) * 64);
      EXPECT_EQ(buffer.size(), sizeof(glm::mat4));
      EXPECT_EQ(buffer.num_elements(), 1);

      EXPECT_NO_FATAL_FAILURE(buffer.buffer_data(vector));
      EXPECT_EQ(buffer.max_size(), sizeof(glm::mat4) * 64);
      EXPECT_EQ(buffer.size(), sizeof(glm::mat4) + sizeof(glm::vec4));
      EXPECT_EQ(buffer.num_elements(), 2);

      EXPECT_NO_FATAL_FAILURE(buffer.buffer_data(transform));
      EXPECT_EQ(buffer.max_size(), sizeof(glm::mat4) * 64);
      EXPECT_EQ(buffer.size(), 2 * sizeof(glm::mat4) + sizeof(glm::vec4));
      EXPECT_EQ(buffer.num_elements(), 3);

      EXPECT_NO_FATAL_FAILURE(buffer.buffer_data(vector));
      EXPECT_EQ(buffer.max_size(), sizeof(glm::mat4) * 64);
      EXPECT_EQ(buffer.size(), 2 * sizeof(glm::mat4) + 2 * sizeof(glm::vec4));
      EXPECT_EQ(buffer.num_elements(), 4);

      EXPECT_NO_FATAL_FAILURE(buffer.buffer_data(transform));
      EXPECT_EQ(buffer.max_size(), sizeof(glm::mat4) * 64);
      EXPECT_EQ(buffer.size(), 3 * sizeof(glm::mat4) + 2 * sizeof(glm::vec4));
      EXPECT_EQ(buffer.num_elements(), 5);

      EXPECT_NO_FATAL_FAILURE(buffer.buffer_data(vector));
      EXPECT_EQ(buffer.max_size(), sizeof(glm::mat4) * 64);
      EXPECT_EQ(buffer.size(), 3 * sizeof(glm::mat4) + 3 * sizeof(glm::vec4));
      EXPECT_EQ(buffer.num_elements(), 6);
    }

    /// these are different items, so we know that the data written is not dependent on memory owned by
    ///   another object (for example coppying over a pointer into the buffer rather than the data pointed to)
    glm::mat4 transform = glm::mat4(1.f);
    glm::vec4 vector = glm::vec4(1.f);

    ASSERT_EQ(buffer.element_size(0), sizeof(glm::mat4));
    const glm::mat4& m0 = buffer.at<glm::mat4>(0);
    EXPECT_EQ(m0, transform);

    ASSERT_EQ(buffer.element_size(1), sizeof(glm::vec4));
    const glm::vec4& v0 = buffer.at<glm::vec4>(1);
    EXPECT_EQ(v0, vector);

    ASSERT_EQ(buffer.element_size(2), sizeof(glm::mat4));
    const glm::mat4& m1 = buffer.at<glm::mat4>(2);
    EXPECT_EQ(m1, transform);

    ASSERT_EQ(buffer.element_size(3), sizeof(glm::vec4));
    const glm::vec4& v1 = buffer.at<glm::vec4>(3);
    EXPECT_EQ(v1, vector);

    ASSERT_EQ(buffer.element_size(4), sizeof(glm::mat4));
    const glm::mat4& m2 = buffer.at<glm::mat4>(4);
    EXPECT_EQ(m2, transform);

    ASSERT_EQ(buffer.element_size(5), sizeof(glm::vec4));
    const glm::vec4& v2 = buffer.at<glm::vec4>(5);
    EXPECT_EQ(v2, vector);

    ASSERT_DEATH(buffer.at<glm::mat4>(1), "");
    ASSERT_DEATH(buffer.at<glm::mat4>(3), "");
    ASSERT_DEATH(buffer.at<glm::mat4>(5), "");
    buffer.zero_mem();
  }

  TEST_F(arena_buffer_test, zero_mem_test) {
    arena_buffer buffer;
    glm::mat4 transform = glm::mat4(1.f);
    glm::vec4 vector = glm::vec4(1.f);

    /// after first write:
    ///   capacity == sizeof(glm::mat4) * 64 == 64 * 64 == 4096
    ///   size == 64
    /// then we shouldn't have to resize again in this test there should be no more resizes
    EXPECT_NO_FATAL_FAILURE(buffer.buffer_data(transform));
    EXPECT_EQ(buffer.max_size(), sizeof(glm::mat4) * 64);
    EXPECT_EQ(buffer.size(), sizeof(glm::mat4));
    EXPECT_EQ(buffer.num_elements(), 1);

    EXPECT_NO_FATAL_FAILURE(buffer.buffer_data(transform));
    EXPECT_EQ(buffer.max_size(), sizeof(glm::mat4) * 64);
    EXPECT_EQ(buffer.size(), 2 * sizeof(glm::mat4));
    EXPECT_EQ(buffer.num_elements(), 2);

    EXPECT_NO_FATAL_FAILURE(buffer.buffer_data(transform));
    EXPECT_EQ(buffer.max_size(), sizeof(glm::mat4) * 64);
    EXPECT_EQ(buffer.size(), 3 * sizeof(glm::mat4));
    EXPECT_EQ(buffer.num_elements(), 3);

    ASSERT_EQ(buffer.element_size(0), sizeof(glm::mat4));
    const glm::mat4& m0 = buffer.at<glm::mat4>(0);
    EXPECT_EQ(m0, transform);

    ASSERT_EQ(buffer.element_size(1), sizeof(glm::mat4));
    const glm::mat4& m1 = buffer.at<glm::mat4>(1);
    EXPECT_EQ(m1, transform);

    ASSERT_EQ(buffer.element_size(2), sizeof(glm::mat4));
    const glm::mat4& m2 = buffer.at<glm::mat4>(2);
    EXPECT_EQ(m2, transform);

    /// should not deallocate but should reset current size so size == 0 but capacity is still the same
    ///  and elements is now 0
    EXPECT_NO_FATAL_FAILURE(buffer.zero_mem());
    EXPECT_EQ(buffer.max_size(), sizeof(glm::mat4) * 64);
    EXPECT_EQ(buffer.size(), 0);
    EXPECT_EQ(buffer.num_elements(), 0);
  }

  TEST_F(arena_buffer_test, copy_constructor) {
    arena_buffer buffer;
    load_buffer_with_matrices(buffer, 4);
    EXPECT_EQ(buffer.max_size(), sizeof(glm::mat4) * 64);
    EXPECT_EQ(buffer.size(), 4 * sizeof(glm::mat4));
    EXPECT_EQ(buffer.num_elements(), 4);

    {
      arena_buffer b2;
      b2 = buffer;

      /// buffer remain unchaged
      EXPECT_EQ(buffer.max_size(), sizeof(glm::mat4) * 64);
      EXPECT_EQ(buffer.size(), 4 * sizeof(glm::mat4));
      EXPECT_EQ(buffer.num_elements(), 4);

      EXPECT_EQ(b2.max_size(), sizeof(glm::mat4) * 64);
      EXPECT_EQ(b2.size(), 4 * sizeof(glm::mat4));
      EXPECT_EQ(b2.num_elements(), 4);
    }

    /// b2 goes out of scope and should have no effect on buffer
    EXPECT_EQ(buffer.max_size(), sizeof(glm::mat4) * 64);
    EXPECT_EQ(buffer.size(), 4 * sizeof(glm::mat4));
    EXPECT_EQ(buffer.num_elements(), 4);
  }

  void foo(const arena_buffer& b) {
    using namespace std::string_view_literals;
    std::cout << b.dump_buffer() << "\n";
  };

  TEST_F(arena_buffer_test, pass_by_const_ref) {
    arena_buffer buffer;
    load_buffer_with_matrices(buffer, 4);
    ASSERT_NO_FATAL_FAILURE(foo(buffer));

    buffer.buffer_data(glm::mat4(1.f));
    ASSERT_NO_FATAL_FAILURE(foo(buffer));

    buffer.zero_mem();
    ASSERT_NO_FATAL_FAILURE(foo(buffer));
  }

}  // namespace other
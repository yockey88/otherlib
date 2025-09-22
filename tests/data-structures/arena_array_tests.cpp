/**
 * \file data-structures/arena_array_tests.cpp
 **/
#include "arena_array_tests.hpp"

#include "data-structures/arena_array.hpp"

namespace other {

  TEST_F(arena_array_tests, basic_operations) {
    arena_array<int, 10> arr;
    EXPECT_EQ(arr.size, 10);
    EXPECT_EQ(arr.capacity, 10 * sizeof(int));

    for (int i = 0; i < 10; i++) {
      arr[i] = i;
    }

    for (int i = 0; i < 10; i++) {
      EXPECT_EQ(arr[i], i) << " at index " << i;
    }
  }

  struct complicated_type {
    complicated_type() : a(0), b(0.0f) {}
    complicated_type(int a, float b, const std::string& s)
        : a(a), b(b), str(s) {}

    int a;
    float b;
    std::string str;

    constexpr auto operator<=>(const complicated_type& other) const = default;
  };

  TEST_F(arena_array_tests, complex_type) {
    arena_array<complicated_type, 5> arr;
    EXPECT_EQ(arr.size, 5);
    EXPECT_EQ(arr.capacity, 5 * sizeof(complicated_type));

    std::vector<complicated_type> expected_vec = {
      complicated_type(1, 1.0f, "one"),
      complicated_type(2, 2.0f, "two"),
      complicated_type(3, 3.0f, "three"),
      complicated_type(4, 4.0f, "four"),
      complicated_type(5, 5.0f, "five"),
    };

    for (int i = 0; i < 5; i++) {
      arr[i] = expected_vec[i];
    }

    for (int i = 0; i < 5; i++) {
      EXPECT_EQ(arr[i], expected_vec[i]) << " at index " << i;
    }
  }

}  // namespace other
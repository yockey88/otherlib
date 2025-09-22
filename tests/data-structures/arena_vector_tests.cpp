/**
 * \file data-structures/arena_vector_tests.cpp
 **/
#include "arena_vector_tests.hpp"

#include "data-structures/arena_vector.hpp"

namespace other {

  TEST_F(arena_vector_tests, reserve) {
    arena_vector<int> vec;
    vec.reserve(5);
    EXPECT_EQ(vec.size, 0);
    EXPECT_EQ(vec.capacity, 5 * sizeof(int));

    for (int i = 0; i < 5; i++) {
      vec.push_back(i);
    }

    int i = 0;
    for (const auto& v : vec) {
      EXPECT_EQ(v, i++) << " at index " << i;
    }

    vec.reserve(20);
    EXPECT_EQ(vec.size, 0);
    EXPECT_GE(vec.capacity, 20 * sizeof(int));
    i = 0;
    for (const auto& v : vec) {
      EXPECT_EQ(v, i++) << " at index " << i;
    }

    vec.clear();
    EXPECT_EQ(vec.size, 0);
    EXPECT_EQ(vec.capacity, 0);
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

  TEST_F(arena_vector_tests, complex_type) {
    std::vector<complicated_type> expected_vec = {
      complicated_type(1, 1.0f, "one"),
      complicated_type(2, 2.0f, "two"),
      complicated_type(3, 3.0f, "three"),
    };

    arena_vector<complicated_type> vec;
    vec.reserve(3);
    EXPECT_EQ(vec.size, 0);
    EXPECT_EQ(vec.capacity, 3 * sizeof(complicated_type));

    for (const auto& v : vec) {
      EXPECT_EQ(v.a, 0);
      EXPECT_EQ(v.b, 0.0f);
      EXPECT_EQ(v.str, "");
    }

    vec.emplace_back(1, 1.0f, "one");
    vec.emplace_back(2, 2.0f, "two");
    vec.emplace_back(3, 3.0f, "three");

    for (size_t i = 0; i < expected_vec.size(); i++) {
      EXPECT_EQ(vec[i], expected_vec[i]) << " at index " << i;
    }

    vec.clear();
    EXPECT_EQ(vec.size, 0);
    EXPECT_EQ(vec.capacity, 0);

    vec.push_back(complicated_type(1, 1.0f, "one"));
    vec.push_back(complicated_type(2, 2.0f, "two"));
    vec.push_back(complicated_type(3, 3.0f, "three"));

    for (size_t i = 0; i < expected_vec.size(); i++) {
      EXPECT_EQ(vec[i], expected_vec[i]) << " at index " << i;
    }

    vec.clear();
    EXPECT_EQ(vec.size, 0);
    EXPECT_EQ(vec.capacity, 0);
  }

}  // namespace other
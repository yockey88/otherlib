/**
 * \file tests/arena/scope_tests.cpp
 **/
#include "scope_tests.hpp"

#include "core/scope.hpp"

namespace other {

  TEST_F(scope_tests, make_scope_basic) {
    struct test_struct {
      int a;
      float b;
    };

    auto ptr = make_scope<test_struct>();
    EXPECT_NE(ptr, nullptr);
    ptr->a = 42;
    ptr->b = 3.14f;

    EXPECT_EQ(ptr->a, 42);
    EXPECT_FLOAT_EQ(ptr->b, 3.14f);
  }

  TEST_F(scope_tests, make_scope_nested) {
    struct inner_struct {
      int x;
    };

    struct outer_struct {
      scope<inner_struct> inner;
      outer_struct() : inner(make_scope<inner_struct>()) {}
    };

    auto outer_ptr = make_scope<outer_struct>();
    EXPECT_NE(outer_ptr, nullptr);
    EXPECT_NE(outer_ptr->inner, nullptr);

    outer_ptr->inner->x = 99;
    EXPECT_EQ(outer_ptr->inner->x, 99);
  }

  TEST_F(scope_tests, scope_move) {
    struct test_struct {
      int value;
    };

    auto original_ptr = make_scope<test_struct>();
    original_ptr->value = 123;

    auto moved_ptr = std::move(original_ptr);
    EXPECT_EQ(moved_ptr->value, 123);
    EXPECT_EQ(original_ptr, nullptr);
  }

}  // namespace other
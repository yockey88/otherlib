/**
 * \file tests/ref/ref_test.cpp
 **/
#include "ref_test.hpp"

namespace other {

  TEST_F(ref_test, default_constructor_creates_null_ref) {
    ref<test_ref_object<int32_t>> default_ref;
    ASSERT_EQ(default_ref.raw_ptr(), nullptr);
    ASSERT_EQ(default_ref.count(), 0);
    ASSERT_FALSE(default_ref);
  }

  TEST_F(ref_test, constructor_with_raw_pointer) {
    ref<test_ref_object<uint32_t>> ptr = new test_ref_object<uint32_t>(42);
    ASSERT_NE(ptr.raw_ptr(), nullptr);
    ASSERT_EQ(ptr.raw_ptr()->count(), 1);
    ASSERT_TRUE(ptr);
  }

  TEST_F(ref_test, copy_constructor) {
    ref<test_ref_object<uint32_t>> ptr = new test_ref_object<uint32_t>(42);
    ASSERT_NE(ptr.raw_ptr(), nullptr);
    ASSERT_EQ(ptr.raw_ptr()->count(), 1);
    ASSERT_TRUE(ptr);

    ASSERT_EQ(ptr->value, 42);
    {
      ref<test_ref_object<uint32_t>> test_ref(ptr);
      ASSERT_NE(test_ref.raw_ptr(), nullptr);

      ASSERT_EQ(ptr.raw_ptr()->count(), 2);
      ASSERT_EQ(test_ref.raw_ptr()->count(), 2);

      ASSERT_EQ(test_ref->value, 42);
    }
    ASSERT_EQ(ptr.raw_ptr()->count(), 1);
  }

  TEST_F(ref_test, move_constructor) {
    ref<test_ref_object<uint32_t>> ptr = new test_ref_object<uint32_t>(42);
    ASSERT_NE(ptr.raw_ptr(), nullptr);
    ASSERT_EQ(ptr.raw_ptr()->count(), 1);
    ASSERT_TRUE(ptr);

    ref<test_ref_object<uint32_t>> test_ref(std::move(ptr));
    ASSERT_EQ(ptr.raw_ptr(), nullptr);
    ASSERT_NE(test_ref.raw_ptr(), nullptr);
    ASSERT_EQ(test_ref.raw_ptr()->count(), 1);
    ASSERT_EQ(test_ref->value, 42);
  }

  TEST_F(ref_test, store_simple_container_type) {
    ref<test_ref_object<std::array<uint32_t, 4>>> container_ref{ new test_ref_object<std::array<uint32_t, 4>>({ 1, 2, 3, 4 }) };
    ASSERT_NE(container_ref.raw_ptr(), nullptr);
    ASSERT_EQ(container_ref.raw_ptr()->count(), 1);
    ASSERT_TRUE(container_ref);

    const auto& data = container_ref->value;
    ASSERT_EQ(data.size(), 4);
    ASSERT_EQ(data[0], 1);
    ASSERT_EQ(data[1], 2);
    ASSERT_EQ(data[2], 3);
    ASSERT_EQ(data[3], 4);
  }

  TEST_F(ref_test, inheritance_constructor) {
    ref<test_ref_object<std::array<int32_t, 4>>> base_ref = new my_thing();
    ASSERT_NE(base_ref.raw_ptr(), nullptr);
    ASSERT_EQ(base_ref.raw_ptr()->count(), 1);
    ASSERT_TRUE(base_ref);
  }

}  // namespace other

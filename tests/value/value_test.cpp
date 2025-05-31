/**
 * \file tests/value/value_test.cpp
 **/
#include "value_test.hpp"

namespace other {

  TEST_F(value_test, default_constructor_creates_empty_value) {
    value default_val;
    ASSERT_TRUE(default_val.is_empty());
    ASSERT_EQ(default_val.size(), 0);
  }

  TEST_F(value_test, constructor_with_int32) {
    value int_val(kTestInt32);

    ASSERT_FALSE(int_val.is_empty());
    ASSERT_EQ(int_val.size(), sizeof(int32_t));
    ASSERT_EQ(int_val.type(), value_type::INT32);

    ASSERT_EQ(static_cast<int32_t>(int_val), kTestInt32);
  }

  TEST_F(value_test, constructor_with_uint64) {
    value uint_val(kTestUint64);

    // ASSERT_FALSE(uint_val.is_empty());
    ASSERT_EQ(uint_val.size(), sizeof(uint64_t));
    // ASSERT_EQ(uint_val.type(), value_type::UINT64);

    // ASSERT_EQ(static_cast<uint64_t>(uint_val), kTestUint64);
  }

  TEST_F(value_test, constructor_with_float) {
    value float_val(kTestFloat);

    ASSERT_FALSE(float_val.is_empty());
    ASSERT_EQ(float_val.size(), sizeof(real_t));
    ASSERT_EQ(float_val.type(), value_type::FLOAT);

    ASSERT_EQ(static_cast<real_t>(float_val), kTestFloat);
  }

  // TEST_F(value_test, constructor_with_double) {
  //   value double_val(kTestDouble);
  //   ASSERT_FALSE(double_val.is_empty());
  //   ASSERT_EQ(double_val.size(), sizeof(double));
  //   ASSERT_EQ(double_val.type(), value_type::DOUBLE);
  //   assert_value_equals(double_val, kTestDouble);
  // }

  // TEST_F(value_test, constructor_with_bool) {
  //   value bool_val(kTestBool);
  //   ASSERT_FALSE(bool_val.is_empty());
  //   ASSERT_EQ(bool_val.size(), sizeof(bool));
  //   ASSERT_EQ(bool_val.type(), value_type::OEBOOL);
  //   assert_value_equals(bool_val, kTestBool);
  // }

  // TEST_F(value_test, constructor_with_char) {
  //   value char_val(kTestChar);
  //   ASSERT_FALSE(char_val.is_empty());
  //   ASSERT_EQ(char_val.size(), sizeof(char));
  //   ASSERT_EQ(char_val.type(), value_type::CHAR);
  //   assert_value_equals(char_val, kTestChar);
  // }

  // TEST_F(value_test, constructor_with_string) {
  //   value string_val(kTestString);
  //   ASSERT_FALSE(string_val.is_empty());
  //   ASSERT_EQ(string_val.size(), kTestString.size());
  //   ASSERT_EQ(string_val.type(), value_type::STRING);
  //   assert_value_equals(string_val, kTestString);
  // }

  // TEST_F(value_test, move_constructor) {
  //   value original_val(kTestInt32);
  //   ASSERT_FALSE(original_val.is_empty());

  //   value moved_val(std::move(original_val));
  //   ASSERT_FALSE(moved_val.is_empty());
  //   ASSERT_EQ(moved_val.size(), sizeof(int32_t));
  //   assert_value_equals(moved_val, kTestInt32);
  // }

  // TEST_F(value_test, copy_constructor) {
  //   value original_val(kTestString);
  //   value copied_val(original_val);

  //   ASSERT_FALSE(copied_val.is_empty());
  //   ASSERT_EQ(copied_val.size(), original_val.size());
  //   ASSERT_EQ(copied_val.type(), original_val.type());
  //   assert_value_equals(copied_val, kTestString);
  //   assert_value_equals(original_val, kTestString);  // Original should remain unchanged
  // }

  // TEST_F(value_test, nullptr_constructor) {
  //   value null_val(nullptr);
  //   ASSERT_TRUE(null_val.is_empty());
  //   ASSERT_EQ(null_val.size(), 0);
  // }

  // TEST_F(value_test, template_move_constructor) {
  //   std::string temp_string = kTestString;
  //   value moved_string_val(std::move(temp_string));

  //   ASSERT_FALSE(moved_string_val.is_empty());
  //   ASSERT_EQ(moved_string_val.type(), value_type::STRING);
  //   assert_value_equals(moved_string_val, kTestString);
  // }

}  // namespace other

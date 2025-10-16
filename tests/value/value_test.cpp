/**
 * \file tests/value/value_test.cpp
 **/
#include "value_test.hpp"

#include "core/defines.hpp"


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

    ASSERT_FALSE(uint_val.is_empty());
    ASSERT_EQ(uint_val.size(), sizeof(uint64_t));
    ASSERT_EQ(uint_val.type(), value_type::UINT64);

    ASSERT_EQ(static_cast<uint64_t>(uint_val), kTestUint64);
  }

  TEST_F(value_test, constructor_with_float) {
    value float_val(kTestFloat);

    ASSERT_FALSE(float_val.is_empty());
    ASSERT_EQ(float_val.size(), sizeof(real_t));
    ASSERT_EQ(float_val.type(), value_type::FLOAT);

    ASSERT_EQ(static_cast<real_t>(float_val), kTestFloat);
  }

  TEST_F(value_test, constructor_with_double) {
    value double_val(kTestDouble);
    ASSERT_FALSE(double_val.is_empty());
    ASSERT_EQ(double_val.size(), sizeof(double));
    ASSERT_EQ(double_val.type(), value_type::DOUBLE);
    ASSERT_EQ(static_cast<double>(double_val), kTestDouble);
  }

  TEST_F(value_test, constructor_with_bool) {
    value bool_val(kTestBool);
    ASSERT_FALSE(bool_val.is_empty());
    ASSERT_EQ(bool_val.size(), sizeof(bool));
    ASSERT_EQ(bool_val.type(), value_type::OEBOOL);
    ASSERT_EQ(static_cast<bool>(bool_val), kTestBool);
  }

  TEST_F(value_test, constructor_with_char) {
    value char_val(kTestChar);
    ASSERT_FALSE(char_val.is_empty());
    ASSERT_EQ(char_val.size(), sizeof(char));
    ASSERT_EQ(char_val.type(), value_type::CHAR);
    ASSERT_EQ(static_cast<char>(char_val), kTestChar);
  }

  TEST_F(value_test, constructor_with_string) {
    value string_val(kTestString);
    ASSERT_FALSE(string_val.is_empty());
    ASSERT_EQ(string_val.size(), kTestString.size());
    ASSERT_EQ(string_val.type(), value_type::STRING);

    std::string str = string_val;
    ASSERT_EQ(str, kTestString);
  }

  TEST_F(value_test, user_data_constructor) {
    struct TestData {
      int a;
      float b;
    };

    TestData data = { 10, 3.14f };
    value user_data_val(data);

    ASSERT_FALSE(user_data_val.is_empty());
    ASSERT_EQ(user_data_val.size(), sizeof(TestData));
    ASSERT_EQ(user_data_val.type(), value_type::USER_TYPE);

    TestData& out_data = user_data_val;
    ASSERT_EQ(out_data.a, data.a);
    ASSERT_EQ(out_data.b, data.b);

    struct TestData2 {
      filepath path1;
      filepath path2;
      std::string my_string;
      int weirdly_placed_int;
      double some_double;
      std::string another_string;
    };

    TestData2 data2 = { "C:/path/to/some/file.txt", "D:/another/path/file2.txt", "hello world", 42, 2.71828, "final string" };
    value user_data_val2(data2);

    ASSERT_FALSE(user_data_val2.is_empty());
    ASSERT_EQ(user_data_val2.size(), sizeof(TestData2));
    ASSERT_EQ(user_data_val2.type(), value_type::USER_TYPE);

    TestData2& out_data2 = user_data_val2;
    ASSERT_EQ(out_data2.path1, data2.path1);
    ASSERT_EQ(out_data2.path2, data2.path2);
    ASSERT_EQ(out_data2.my_string, data2.my_string);
    ASSERT_EQ(out_data2.weirdly_placed_int, data2.weirdly_placed_int);
    ASSERT_EQ(out_data2.some_double, data2.some_double);
    ASSERT_EQ(out_data2.another_string, data2.another_string);
  }

}  // namespace other

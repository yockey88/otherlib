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

  TEST_F(value_test, assignment_constructor_from_string) {
    value string_val = kTestString;
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

    struct TestData3 {
      filepath path;
      std::string name;
      int id;
      std::vector<float> values;
      std::vector<std::string> labels;
    };
    TestData3 data3 = {
      "E:/data/path/datafile.dat",
      "TestData3Object",
      7,
      { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f },
      { "first", "second", "third" }
    };

    value user_data_val3(data3);
    ASSERT_FALSE(user_data_val3.is_empty());
    ASSERT_EQ(user_data_val3.size(), sizeof(TestData3));
    ASSERT_EQ(user_data_val3.type(), value_type::USER_TYPE);

    TestData3& out_data3 = user_data_val3;
    ASSERT_EQ(out_data3.path, data3.path);
    ASSERT_EQ(out_data3.name, data3.name);
    ASSERT_EQ(out_data3.id, data3.id);
    ASSERT_EQ(out_data3.values, data3.values);
    ASSERT_EQ(out_data3.labels, data3.labels);
  }

  TEST_F(value_test, user_data_copy_ctor) {
    struct TestData {
      filepath path;
      std::string name;
      int id;
      std::vector<float> values;
      std::vector<std::string> labels;
    };

    TestData data = {
      "C:/path/to/some/file.txt",
      "TestDataObject",
      42,
      { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f },
      { "first", "second", "third" }
    };

    value user_data_val(data);
    ASSERT_FALSE(user_data_val.is_empty());
    ASSERT_EQ(user_data_val.size(), sizeof(TestData));
    ASSERT_EQ(user_data_val.type(), value_type::USER_TYPE);

    TestData& out_data = user_data_val;
    ASSERT_EQ(out_data.path, data.path);
    ASSERT_EQ(out_data.name, data.name);
    ASSERT_EQ(out_data.id, data.id);
    ASSERT_EQ(out_data.values, data.values);
    ASSERT_EQ(out_data.labels, data.labels);

    value val2 = user_data_val;  // copy constructor
    ASSERT_FALSE(val2.is_empty());
    ASSERT_EQ(val2.size(), sizeof(TestData));
    ASSERT_EQ(val2.type(), value_type::USER_TYPE);

    TestData& out_data2 = val2;
    ASSERT_EQ(out_data2.path, data.path);
    ASSERT_EQ(out_data2.name, data.name);
    ASSERT_EQ(out_data2.id, data.id);
    ASSERT_EQ(out_data2.values, data.values);
    ASSERT_EQ(out_data2.labels, data.labels);

    auto tfunc = [](const value& val) -> ::testing::AssertionResult {
      value my_val = val;
      if (my_val.is_empty()) {
        return ::testing::AssertionFailure() << "Value is empty";
      }
      if (my_val.type() != value_type::USER_TYPE) {
        return ::testing::AssertionFailure() << "Value type is not USER_TYPE";
      }

      TestData& retrieved_data = my_val;
      if (retrieved_data.path != "C:/path/to/some/file.txt") {
        return ::testing::AssertionFailure() << "Path does not match";
      }
      if (retrieved_data.name != "TestDataObject") {
        return ::testing::AssertionFailure() << "Name does not match";
      }
      if (retrieved_data.id != 42) {
        return ::testing::AssertionFailure() << "ID does not match";
      }
      if (retrieved_data.values != std::vector<float>({ 1.0f, 2.0f, 3.0f, 4.0f, 5.0f })) {
        return ::testing::AssertionFailure() << "Values vector does not match";
      }
      if (retrieved_data.labels != std::vector<std::string>({ "first", "second", "third" })) {
        return ::testing::AssertionFailure() << "Labels vector does not match";
      }
      return ::testing::AssertionSuccess();
    };

    ASSERT_TRUE(tfunc(val2));
  }

  TEST_F(value_test, opaque_handle_constructor) {
    int dummy_data = 12345;
    void* opaque_ptr = static_cast<void*>(&dummy_data);

    value opaque_val = value::create_opaque_handle(opaque_ptr);

    ASSERT_FALSE(opaque_val.is_empty());
    ASSERT_EQ(opaque_val.size(), sizeof(void*));
    ASSERT_EQ(opaque_val.type(), value_type::OPAQUE_HANDLE);

    void* retrieved_ptr = opaque_val;
    ASSERT_EQ(retrieved_ptr, opaque_ptr);
    ASSERT_EQ(*static_cast<int*>(retrieved_ptr), dummy_data);
  }

  ::testing::AssertionResult const_ref_foo(const std::string& str) {
    try {
      if (str != "hello!") {
        return ::testing::AssertionFailure() << "Expected 'hello!', got '" << str << "'";
      }
      return ::testing::AssertionSuccess();
    } catch (...) {
      return ::testing::AssertionFailure() << "Exception thrown when passing string by const reference";
    }
  }

  TEST_F(value_test, pass_string_by_const_ref) {
    using namespace std::string_literals;
    value test_value = "hello!"s;
    ASSERT_TRUE(const_ref_foo(test_value));
  }

}  // namespace other

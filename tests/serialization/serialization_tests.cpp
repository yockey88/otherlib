/**
 * \file tests/serialization/serialization_tests.cpp
 **/
#ifndef OTHER_TESTS_SERIALIZATION_SERIALIZATION_TESTS_CPP
#define OTHER_TESTS_SERIALIZATION_SERIALIZATION_TESTS_CPP

#include "serialization_tests.hpp"

#include "serialization/serializer.hpp"

namespace other {

  void print_buffer_as_long_hex_number(std::span<const uint8_t> buffer) {
    std::string hex_string;
    for (uint8_t byte : buffer) {
      hex_string += std::format("{:02x}", byte);
    }
    CORE_LOG_INFO("Buffer as hex: {}", hex_string);
  }

  TEST_F(serialization_tests, simple_vec3_serialization) {
    glm::vec3 vec{ 1.0f, 2.0f, 3.0f };

    ostd::vector<uint8_t> bytes = {};
    {
      value_type val = get_value_type<glm::vec3>();
      uint32_t size = sizeof(glm::vec3);
      const uint8_t* type_data = reinterpret_cast<const uint8_t*>(&val);
      const uint8_t* size_data = reinterpret_cast<const uint8_t*>(&size);
      const uint8_t* value_data = reinterpret_cast<const uint8_t*>(glm::value_ptr(vec));
      bytes.append_range(std::span(type_data, sizeof(value_type)));
      bytes.append_range(std::span(size_data, sizeof(uint32_t)));
      bytes.append_range(std::span(value_data, sizeof(glm::vec3)));
    }
    print_buffer_as_long_hex_number(bytes);

    serializer ser{};
    ser.write_to_bytes(vec);
    print_buffer_as_long_hex_number(ser.data);

    ASSERT_EQ(ser.data, bytes);
    ASSERT_EQ(value_type(ser.peek_byte()), value_type::VEC3);

    opt<glm::vec3> res = ser.read_from_bytes<glm::vec3>();
    ASSERT_TRUE(res.has_value());
    ASSERT_EQ(res->x, vec.x);
    ASSERT_EQ(res->y, vec.y);
    ASSERT_EQ(res->z, vec.z);
  }

  TEST_F(serialization_tests, simple_int_serialization) {
    int value = 42;

    ostd::vector<uint8_t> bytes = {};
    {
      value_type val = get_value_type<int>();
      uint32_t size = sizeof(int);
      const uint8_t* type_data = reinterpret_cast<const uint8_t*>(&val);
      const uint8_t* size_data = reinterpret_cast<const uint8_t*>(&size);
      const uint8_t* value_data = reinterpret_cast<const uint8_t*>(&value);
      bytes.append_range(std::span(type_data, sizeof(value_type)));
      bytes.append_range(std::span(size_data, sizeof(uint32_t)));
      bytes.append_range(std::span(value_data, sizeof(int)));
    }
    print_buffer_as_long_hex_number(bytes);

    serializer ser{};
    ser.write_to_bytes(value);
    print_buffer_as_long_hex_number(ser.data);

    ASSERT_EQ(ser.data, bytes);
    ASSERT_EQ(value_type(ser.peek_byte()), value_type::INT32);

    opt<int> res = ser.read_from_bytes<int>();
    ASSERT_TRUE(res.has_value());
    ASSERT_EQ(res.value(), value);
  }

  TEST_F(serialization_tests, simple_string_serialization) {
    std::string value = "Hello, world!";

    ostd::vector<uint8_t> bytes = {};
    {
      value_type val = get_value_type<std::string>();
      uint32_t size = static_cast<uint32_t>(value.size());
      const uint8_t* type_data = reinterpret_cast<const uint8_t*>(&val);
      const uint8_t* size_data = reinterpret_cast<const uint8_t*>(&size);
      const uint8_t* value_data = reinterpret_cast<const uint8_t*>(value.data());
      bytes.append_range(std::span(type_data, sizeof(value_type)));
      bytes.append_range(std::span(size_data, sizeof(uint32_t)));
      bytes.append_range(std::span(value_data, value.size()));
    }
    print_buffer_as_long_hex_number(bytes);

    serializer ser{};
    ser.write_to_bytes(value);
    print_buffer_as_long_hex_number(ser.data);

    ASSERT_EQ(ser.data, bytes);
    ASSERT_EQ(value_type(ser.peek_byte()), value_type::STRING);

    opt<std::string> res = ser.read_from_bytes<std::string>();
    ASSERT_TRUE(res.has_value());
    ASSERT_EQ(res.value(), value);
  }

  TEST_F(serialization_tests, simple_float_serialization) {
    float value = 3.14f;

    ostd::vector<uint8_t> bytes = {};
    {
      value_type val = get_value_type<float>();
      uint32_t size = sizeof(float);
      const uint8_t* type_data = reinterpret_cast<const uint8_t*>(&val);
      const uint8_t* size_data = reinterpret_cast<const uint8_t*>(&size);
      const uint8_t* value_data = reinterpret_cast<const uint8_t*>(&value);
      bytes.append_range(std::span(type_data, sizeof(value_type)));
      bytes.append_range(std::span(size_data, sizeof(uint32_t)));
      bytes.append_range(std::span(value_data, sizeof(float)));
    }
    print_buffer_as_long_hex_number(bytes);

    serializer ser{};
    ser.write_to_bytes(value);
    print_buffer_as_long_hex_number(ser.data);

    ASSERT_EQ(ser.data, bytes);
    ASSERT_EQ(value_type(ser.peek_byte()), value_type::FLOAT);

    opt<float> res = ser.read_from_bytes<float>();
    ASSERT_TRUE(res.has_value());
    ASSERT_EQ(res.value(), value);
  }

  TEST_F(serialization_tests, simple_double_serialization) {
    double value = 3.141592653589793;

    ostd::vector<uint8_t> bytes = {};
    {
      value_type val = get_value_type<double>();
      uint32_t size = sizeof(double);
      const uint8_t* type_data = reinterpret_cast<const uint8_t*>(&val);
      const uint8_t* size_data = reinterpret_cast<const uint8_t*>(&size);
      const uint8_t* value_data = reinterpret_cast<const uint8_t*>(&value);
      bytes.append_range(std::span(type_data, sizeof(value_type)));
      bytes.append_range(std::span(size_data, sizeof(uint32_t)));
      bytes.append_range(std::span(value_data, sizeof(double)));
    }
    print_buffer_as_long_hex_number(bytes);

    serializer ser{};
    ser.write_to_bytes(value);
    print_buffer_as_long_hex_number(ser.data);

    ASSERT_EQ(ser.data, bytes);
    ASSERT_EQ(value_type(ser.peek_byte()), value_type::DOUBLE);

    opt<double> res = ser.read_from_bytes<double>();
    ASSERT_TRUE(res.has_value());
    ASSERT_EQ(res.value(), value);
  }

  TEST_F(serialization_tests, simple_byte_buffer_serialization) {
    std::vector<uint8_t> value = { 0xDE, 0xAD, 0xBE, 0xEF };

    ostd::vector<uint8_t> bytes = {};
    {
      value_type val = get_value_type<std::span<const uint8_t>>();
      uint32_t size = static_cast<uint32_t>(value.size());
      const uint8_t* type_data = reinterpret_cast<const uint8_t*>(&val);
      const uint8_t* size_data = reinterpret_cast<const uint8_t*>(&size);
      const uint8_t* value_data = value.data();
      bytes.append_range(std::span(type_data, sizeof(value_type)));
      bytes.append_range(std::span(size_data, sizeof(uint32_t)));
      bytes.append_range(std::span(value_data, value.size()));
    }
    print_buffer_as_long_hex_number(bytes);

    serializer ser{};
    ser.write_to_bytes(std::span<const uint8_t>(value.data(), value.size()));
    print_buffer_as_long_hex_number(ser.data);

    // ASSERT_EQ(ser.data, bytes);
    // ASSERT_EQ(value_type(ser.peek_byte()), value_type::BYTE_BUFFER);

    // opt<ostd::vector<uint8_t>> res = ser.read_from_bytes<ostd::vector<uint8_t>>();
    // ASSERT_TRUE(res.has_value());
    // ASSERT_EQ(res.value(), value);
  }

}  // namespace other

#endif  // OTHER_TESTS_SERIALIZATION_SERIALIZATION_TESTS_CPP
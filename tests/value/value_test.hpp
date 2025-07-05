/**
 * \file tests/value/value_test.hpp
 **/
#ifndef OTHER_VALUE_TEST_HPP
#define OTHER_VALUE_TEST_HPP

#include <cstdint>
#include <string>

#include <gtest/gtest.h>

#include "core/defines.hpp"
#include "core/value.hpp"

#include "other_test.hpp"

namespace other {

  class value_test : public other_test {
   protected:
    static constexpr int32_t kTestInt32 = 42;
    static constexpr uint64_t kTestUint64 = 12345ULL;
    static constexpr real_t kTestFloat = 3.14159f;
    static constexpr double kTestDouble = 2.71828;
    static constexpr bool kTestBool = true;
    static constexpr char kTestChar = 'A';
    static inline const std::string kTestString = "test_string";
  };

}  // namespace other

#endif  // OTHER_VALUE_TEST_HPP

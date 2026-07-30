/**
 * \file tests/serialization/serialization_tests.hpp
 **/
#ifndef OTHER_TESTS_SERIALIZATION_SERIALIZATION_TESTS_HPP
#define OTHER_TESTS_SERIALIZATION_SERIALIZATION_TESTS_HPP

#include "other_test.hpp"

namespace other {

  class serialization_tests : public other_test {
   public:
    bool script_and_physics() const override { return true; }
  };

}  // namespace other

#endif  // OTHER_TESTS_SERIALIZATION_SERIALIZATION_TESTS_HPP
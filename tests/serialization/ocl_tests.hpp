/**
 * \file tests/serialization/ocl_tests.hpp
 **/
#ifndef OTHERLIB_TESTS_SERIALIZATION_OCL_TESTS_HPP
#define OTHERLIB_TESTS_SERIALIZATION_OCL_TESTS_HPP

#include "ocl/object_parser.hpp"
#include "other_test.hpp"

namespace other {

  class ocl_tests : public other_test {
   public:
    virtual ~ocl_tests() override = default;
  };

}  // namespace other

#endif  // OTHERLIB_TESTS_SERIALIZATION_OCL_TESTS_HPP
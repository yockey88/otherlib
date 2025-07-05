/**
 * \file tests/ref/ref_test.hpp
 **/
#ifndef OTHER_REF_TEST_HPP
#define OTHER_REF_TEST_HPP

#include <memory>

#include "core/defines.hpp"
#include "core/ref.hpp"
#include "core/ref_counted.hpp"

#include "other_test.hpp"

namespace other {

  struct test_object : public ref_counted {
    test_object() : value(0) {}
    test_object(int32_t value) : value(value) {}
    virtual ~test_object() = default;

    int32_t value;
  };

  template <typename T>
  struct test_ref_object : public ref_counted {
    explicit test_ref_object(T value) : value(value) {}
    virtual ~test_ref_object() = default;

    T value;
  };

  struct my_thing : test_ref_object<std::array<int32_t, 4>> {
    my_thing() : test_ref_object({ 0, 2, 4, 8 }) {}
    ~my_thing() override = default;
  };

  class ref_test : public other_test {
   protected:
  };

}  // namespace other

#endif  // OTHER_REF_TEST_HPP

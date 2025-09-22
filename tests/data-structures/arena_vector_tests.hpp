/**
 * \file data-structures/arena_vector_tests.hpp
 **/
#ifndef OTHER_CORE_DATA_STRUCTURES_ARENA_VECTOR_TESTS_HPP
#define OTHER_CORE_DATA_STRUCTURES_ARENA_VECTOR_TESTS_HPP

#include "other_test.hpp"

namespace other {

  class arena_vector_tests : public other_test {
   public:
    void SetUp() override {
      /// reset because we are testing the arena here
      subsystem<arena>::get()->shutdown();
      subsystem<arena>::get();
    }

    void TearDown() override {
      subsystem<arena>::get()->shutdown();
    }
  };

}  // namespace other

#endif  // OTHER_CORE_DATA_STRUCTURES_ARENA_VECTOR_TESTS_HPP
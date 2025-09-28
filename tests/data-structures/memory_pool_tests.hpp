/**
 * \file data-structures/memory_pool_tests.hpp
 **/
#ifndef OTHER_TESTS_DATA_STRUCTURES_MEMORY_POOL_TESTS_HPP
#define OTHER_TESTS_DATA_STRUCTURES_MEMORY_POOL_TESTS_HPP

#include "other_test.hpp"

namespace other {

  class memory_pool_tests : public other_test {
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

#endif  // OTHER_TESTS_DATA_STRUCTURES_MEMORY_POOL_TESTS_HPP
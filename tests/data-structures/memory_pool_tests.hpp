/**
 * \file data-structures/memory_pool_tests.hpp
 **/
#ifndef OTHER_TESTS_DATA_STRUCTURES_MEMORY_POOL_TESTS_HPP
#define OTHER_TESTS_DATA_STRUCTURES_MEMORY_POOL_TESTS_HPP

#include "core/memory_pool.hpp"

#include "other_test.hpp"

namespace other {

  /// counts constructions/destructions so tests can assert the pool actually runs them
  struct pool_probe {
    inline static int32_t constructed = 0;
    inline static int32_t destroyed = 0;

    pool_probe() { constructed++; }
    ~pool_probe() { destroyed++; }

    static void reset() {
      constructed = 0;
      destroyed = 0;
    }
  };

  class memory_pool_tests : public other_test {
   public:
  };

}  // namespace other

#endif  // OTHER_TESTS_DATA_STRUCTURES_MEMORY_POOL_TESTS_HPP

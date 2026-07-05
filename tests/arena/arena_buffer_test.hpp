/**
 * \file test/arena/arena_buffer_test.hpp
 **/
#ifndef OTHER_TESTS_ARENA_BUFFER_TEST_HPP
#define OTHER_TESTS_ARENA_BUFFER_TEST_HPP

#include <cstdint>
#include <vector>

#include "core/arena_buffer.hpp"
#include "memory/arena.hpp"

#include "other_test.hpp"


namespace other {

  class arena_buffer_test : public other_test {
   protected:
    void load_buffer_with_matrices(arena_buffer& buffer, size_t num_matrices);
  };

}  // namespace other

#endif  // OTHER_TESTS_ARENA_BUFFER_TEST_HPP
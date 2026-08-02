/**
 * \file tests/harness/src/memory_sampler.hpp
 **/
#ifndef OTHER_TESTS_HARNESS_MEMORY_SAMPLER_HPP
#define OTHER_TESTS_HARNESS_MEMORY_SAMPLER_HPP

#include <cstdint>

#include "core/defines.hpp"

namespace other {

  struct memory_sample {
    double elapsed_seconds = 0.0;
    uint64_t frame_index = 0;

    /// engine arena counters (see arena::stats)
    size_t arena_total_allocations = 0;
    size_t arena_live_allocations = 0;
    size_t arena_requested_memory = 0;
    size_t arena_used_memory = 0;

    /// OS process counters; noisy compared to the arena numbers, recorded for context
    size_t process_working_set = 0;
    size_t process_private_bytes = 0;
  };

  memory_sample take_memory_sample(double elapsed_seconds, uint64_t frame_index);

}  // namespace other

#endif  // OTHER_TESTS_HARNESS_MEMORY_SAMPLER_HPP

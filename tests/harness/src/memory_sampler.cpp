/**
 * \file tests/harness/src/memory_sampler.cpp
 **/
#include "memory_sampler.hpp"

#include "core/subsystem.hpp"
#include "memory/arena.hpp"

#ifdef OTHER_ENVIRONMENT_WINDOWS
  #define PSAPI_VERSION 2
  #include <Windows.h>

  #include <psapi.h>
#endif

namespace other {

  memory_sample take_memory_sample(double elapsed_seconds, uint64_t frame_index) {
    memory_sample sample{
      .elapsed_seconds = elapsed_seconds,
      .frame_index = frame_index,
    };

    arena::stats arena_stats = subsystem<arena>::get()->get_stats();
    sample.arena_total_allocations = arena_stats.total_allocations;
    sample.arena_live_allocations = arena_stats.live_allocations;
    sample.arena_requested_memory = arena_stats.requested_memory;
    sample.arena_used_memory = arena_stats.used_memory;

#ifdef OTHER_ENVIRONMENT_WINDOWS
    PROCESS_MEMORY_COUNTERS_EX counters{};
    counters.cb = sizeof(counters);
    if (K32GetProcessMemoryInfo(GetCurrentProcess(), reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&counters), sizeof(counters))) {
      sample.process_working_set = counters.WorkingSetSize;
      sample.process_private_bytes = counters.PrivateUsage;
    }
#endif

    return sample;
  }

}  // namespace other

/**
 * \file core/profiler.hpp
 **/
#ifndef OTHER_CORE_PROFILER_HPP
#define OTHER_CORE_PROFILER_HPP

#include "core/build_config.hpp"

#ifdef OTHER_PROFILE_BUILD
  #define OTHER_INCLUDE_PROFILING
#endif
#ifdef OTHER_PROFILED_BUILD
  #define OTHER_INCLUDE_PROFILING
#endif

#ifdef OTHER_INCLUDE_PROFILING

  #include <cstring>

  #include "core/profiler_backend.hpp"

  #define OTHER_PROFILER_CONCAT2(a, b) a##b
  #define OTHER_PROFILER_CONCAT(a, b) OTHER_PROFILER_CONCAT2(a, b)

  /// zones route through the per-image backend table (profiler_backend.hpp); no module
  ///  outside other-core links tracy, so unloadable plugins never embed a client
  #define OTHER_PROFILE_ZONE(zone_name)                                                                   \
    static constexpr ::other::profiling::source_location OTHER_PROFILER_CONCAT(_other_srcloc_, __LINE__){ \
      zone_name, __FUNCTION__, __FILE__, (uint32_t)__LINE__, 0                                            \
    };                                                                                                    \
    ::other::profiling::scoped_zone _other_profile_zone { &OTHER_PROFILER_CONCAT(_other_srcloc_, __LINE__) }

  #define MARK_FRAME() ::other::profiling::emit_frame_mark(nullptr)
  #define MARK_NAMED_FRAME(name) ::other::profiling::emit_frame_mark(name)

  #define PROFILE_SCOPE() OTHER_PROFILE_ZONE(nullptr)
  #define PROFILE_SECTION(name) OTHER_PROFILE_ZONE(name)

  /// sub-microsecond hot-path zones (millions per capture) — opt in via OTHER_PROFILE_VERBOSE
  #ifdef OTHER_PROFILE_VERBOSE
    #define PROFILE_SECTION_VERBOSE(name) OTHER_PROFILE_ZONE(name)
  #else
    #define PROFILE_SECTION_VERBOSE(name) ((void)0)
  #endif

  #define ADD_PROFILE_TAG(name) _other_profile_zone.text(name, strlen(name))
  #define ADD_PROFILE_MESSAGE(message, size) ::other::profiling::emit_message(message, size)
  #define PROFILE_PLOT_VALUE(name, value) ::other::profiling::emit_plot(name, static_cast<double>(value))

  #define PROFILE_ALLOCATION(p, size) ::other::profiling::emit_memory_alloc(p, size)
  #define PROFILE_DEALLOCATION(p) ::other::profiling::emit_memory_free(p)

  /// lockable instrumentation retired with the seam: tracy lockables cannot cross the image
  ///  boundary, so mutexes stay plain types in every config
  #define PROFILE_MUTEX_TYPE(type, name) type name
  #define LOCK_MUTEX(type, name) \
    std::lock_guard<type> lck { name }

#else

  #define MARK_FRAME() ((void)0)
  #define MARK_NAMED_FRAME(name) ((void)0)

  #define PROFILE_SCOPE() ((void)0)
  #define PROFILE_SECTION(name) ((void)0)
  #define PROFILE_SECTION_VERBOSE(name) ((void)0)

  #define ADD_PROFILE_TAG(name) ((void)0)
  #define ADD_PROFILE_MESSAGE(message, size) ((void)0)
  #define PROFILE_PLOT_VALUE(name, value) ((void)0)

  #define PROFILE_ALLOCATION(ptr, size) (void)0
  #define PROFILE_DEALLOCATION(ptr) (void)0

  #define PROFILE_MUTEX_TYPE(type, name) type name
  #define LOCK_MUTEX(type, value) \
    std::lock_guard<type> lck { value }

#endif  // OTHER_PROFILE_BUILD
#endif  // OTHER_CORE_PROFILER_HPP
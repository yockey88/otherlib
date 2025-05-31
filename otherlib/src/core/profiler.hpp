/**
 * \file core/profiler.hpp
 **/
#ifndef OTHER_CORE_PROFILER_HPP
#define OTHER_CORE_PROFILER_HPP

#ifdef OTHER_PROFILE_BUILD
  #error "Profiler is not supported yet."
#else

  #define PROFILE_ALLOCATION(ptr, size) (void)0
  #define PROFILE_DEALLOCATION(ptr) (void)0

#endif  // OTHER_PROFILE_BUILD

#endif  // OTHER_CORE_PROFILER_HPP
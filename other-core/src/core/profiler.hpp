/**
 * \file core/profiler.hpp
 **/
#ifndef OTHER_CORE_PROFILER_HPP
#define OTHER_CORE_PROFILER_HPP
#ifdef OTHER_PROFILE_BUILD

  #include "tracy/tracy/Tracy.hpp"

  #define MARK_FRAME() FrameMark
  #define MARK_NAMED_FRAME(name) FrameMarkNamed(name)

  #define PROFILE_SCOPE() ZoneScoped
  #define PROFILE_SECTION(name) ZoneScopedN(name)

  #define ADD_PROFILE_TAG(name) ZoneText(name, strlen(name))
  #define ADD_PROFILE_MESSAGE(message, size) TracyMessageS(message, size)
  #define PROFILE_PLOT_VALUE(name, value) TracyPlot(name, value)

  #define PROFILE_ALLOCATION(p, size) TracyAlloc(p, size)
  #define PROFILE_DEALLOCATION(p) TracyFree(p)

#else

  #define MARK_FRAME() ((void)0)
  #define MARK_NAMED_FRAME(name) ((void)0)

  #define PROFILE_SCOPE() ((void)0)
  #define PROFILE_SECTION(name) ((void)0)

  #define ADD_PROFILE_TAG(name) ((void)0)
  #define ADD_PROFILE_MESSAGE(message, size) ((void)0)
  #define PROFILE_PLOT_VALUE(name, value) ((void)0)

  #define PROFILE_ALLOCATION(ptr, size) (void)0
  #define PROFILE_DEALLOCATION(ptr) (void)0

#endif  // OTHER_PROFILE_BUILD
#endif  // OTHER_CORE_PROFILER_HPP
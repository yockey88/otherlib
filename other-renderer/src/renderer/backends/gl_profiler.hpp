/**
 * \file renderer/backends/gl_profiler.hpp
 **/
#ifndef OTHER_RENDERER_RENDERER_BACKENDS_GL_PROFILER_HPP
#define OTHER_RENDERER_RENDERER_BACKENDS_GL_PROFILER_HPP

#include "core/profiler.hpp"
#include "core/profiler_backend.hpp"

/// gpu timestamp zones, self-noop outside Profile/ProfileD and until the exe installs the
///  gpu table (gl_profiler_backend.cpp): CONTEXT once after gl init, COLLECT once after
///  swap, SECTION scoped around gl submission
#ifdef OTHER_INCLUDE_PROFILING

  #define PROFILE_GPU_CONTEXT() ::other::profiling::emit_gpu_context_create()
  #define PROFILE_GPU_SECTION(name)                                                                       \
    static constexpr ::other::profiling::source_location OTHER_PROFILER_CONCAT(_other_gpu_srcloc_, __LINE__){ \
      name, __FUNCTION__, __FILE__, (uint32_t)__LINE__, 0                                                 \
    };                                                                                                    \
    ::other::profiling::scoped_gpu_zone _other_gpu_profile_zone { &OTHER_PROFILER_CONCAT(_other_gpu_srcloc_, __LINE__) }
  #define PROFILE_GPU_COLLECT() ::other::profiling::emit_gpu_collect()

#else

  #define PROFILE_GPU_CONTEXT() ((void)0)
  #define PROFILE_GPU_SECTION(name) ((void)0)
  #define PROFILE_GPU_COLLECT() ((void)0)

#endif  // OTHER_INCLUDE_PROFILING

#endif  // OTHER_RENDERER_RENDERER_BACKENDS_GL_PROFILER_HPP

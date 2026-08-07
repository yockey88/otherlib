/**
 * \file renderer/backends/gl_profiler.hpp
 **/
#ifndef OTHER_RENDERER_RENDERER_BACKENDS_GL_PROFILER_HPP
#define OTHER_RENDERER_RENDERER_BACKENDS_GL_PROFILER_HPP

#include <glad/glad.h>

#include "tracy/tracy/TracyOpenGL.hpp"

/// gpu timestamp zones, self-noop outside Profile/ProfileD: CONTEXT once after gl init,
///  COLLECT once after swap, SECTION scoped around gl submission
#define PROFILE_GPU_CONTEXT() TracyGpuContext
#define PROFILE_GPU_SECTION(name) TracyGpuZone(name)
#define PROFILE_GPU_COLLECT() TracyGpuCollect

#endif  // OTHER_RENDERER_RENDERER_BACKENDS_GL_PROFILER_HPP

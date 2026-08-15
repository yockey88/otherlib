/**
 * \file renderer/backends/gl_profiler_backend.hpp
 **/
#ifndef OTHER_RENDERER_RENDERER_BACKENDS_GL_PROFILER_BACKEND_HPP
#define OTHER_RENDERER_RENDERER_BACKENDS_GL_PROFILER_BACKEND_HPP

namespace other {
  namespace profiling {

    /// installs the gl gpu-zone table into this image. exe entry points only: this TU is
    ///  the renderer's one tracy reference, and nothing reachable from plugin images may
    ///  pull it (see core/profiler_backend.hpp)
    void initialize_host_gpu_backend();

  }  // namespace profiling
}  // namespace other

#endif  // OTHER_RENDERER_RENDERER_BACKENDS_GL_PROFILER_BACKEND_HPP

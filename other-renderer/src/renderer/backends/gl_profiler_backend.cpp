/**
 * \file renderer/backends/gl_profiler_backend.cpp
 *
 * the renderer's only tracy reference. reached exclusively from exe entry points
 *  (initialize_host_gpu_backend), so the linker can never drag it — and the client with
 *  it — into a plugin image through the renderer objects the handshake pulls
 **/
#include "renderer/backends/gl_profiler_backend.hpp"

#include "core/profiler.hpp"
#include "core/profiler_backend.hpp"

#ifdef OTHER_INCLUDE_PROFILING

  #include <cstddef>
  #include <new>

  #include <glad/glad.h>

  #include "tracy/tracy/TracyOpenGL.hpp"

namespace other {
  namespace profiling {

    static_assert(sizeof(source_location) == sizeof(tracy::SourceLocationData));
    static_assert(offsetof(source_location, name) == offsetof(tracy::SourceLocationData, name));
    static_assert(offsetof(source_location, function) == offsetof(tracy::SourceLocationData, function));
    static_assert(offsetof(source_location, file) == offsetof(tracy::SourceLocationData, file));
    static_assert(offsetof(source_location, line) == offsetof(tracy::SourceLocationData, line));
    static_assert(offsetof(source_location, color) == offsetof(tracy::SourceLocationData, color));

    namespace {

      /// gpu zones are strictly nested on the render thread, so scope objects live in a
      ///  fixed lifo; past kMaxGpuZoneDepth the begin/end pair still balances but records nothing
      constexpr size_t kMaxGpuZoneDepth = 16;
      alignas(tracy::GpuCtxScope) std::byte gpu_zone_slots[kMaxGpuZoneDepth][sizeof(tracy::GpuCtxScope)];
      size_t gpu_zone_depth = 0;

      void gpu_context_create() {
        TracyGpuContext;
      }

      void gpu_zone_begin(const source_location* loc) {
        if (gpu_zone_depth >= kMaxGpuZoneDepth) {
          gpu_zone_depth++;
          return;
        }
        new (gpu_zone_slots[gpu_zone_depth]) tracy::GpuCtxScope(
          reinterpret_cast<const tracy::SourceLocationData*>(loc), TRACY_CALLSTACK, true);
        gpu_zone_depth++;
      }

      void gpu_zone_end() {
        if (gpu_zone_depth == 0) {
          return;
        }
        gpu_zone_depth--;
        if (gpu_zone_depth < kMaxGpuZoneDepth) {
          reinterpret_cast<tracy::GpuCtxScope*>(gpu_zone_slots[gpu_zone_depth])->~GpuCtxScope();
        }
      }

      void gpu_collect() {
        TracyGpuCollect;
      }

      constexpr gpu_profiler_backend kGlGpuTable{
        gpu_context_create, gpu_zone_begin, gpu_zone_end, gpu_collect
      };

    }  // namespace

    void initialize_host_gpu_backend() {
      install_gpu_backend(&kGlGpuTable);
    }

  }  // namespace profiling
}  // namespace other

#else  // !OTHER_INCLUDE_PROFILING

namespace other {
  namespace profiling {

    void initialize_host_gpu_backend() {}

  }  // namespace profiling
}  // namespace other

#endif  // OTHER_INCLUDE_PROFILING

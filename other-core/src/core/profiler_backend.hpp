/**
 * \file core/profiler_backend.hpp
 **/
#ifndef OTHER_CORE_PROFILER_BACKEND_HPP
#define OTHER_CORE_PROFILER_BACKEND_HPP

#include <atomic>
#include <cstddef>
#include <cstdint>

namespace other {
  namespace profiling {

    /// layout mirrors tracy's ___tracy_source_location_data / ___tracy_c_zone_context
    ///  (asserted in profiler_backend.cpp) so only that one TU ever includes tracy
    struct source_location {
      const char* name;
      const char* function;
      const char* file;
      uint32_t line;
      uint32_t color;
    };

    struct zone_ctx {
      uint32_t id;
      int32_t active;
    };

    /// per-image indirection to the single host-owned tracy client; plugins receive the
    ///  copying table through the plugin handshake, so their events never reference
    ///  plugin-image memory after FreeLibrary
    struct profiler_backend {
      zone_ctx (*zone_begin)(const source_location* loc);
      void (*zone_end)(zone_ctx ctx);
      void (*zone_text)(zone_ctx ctx, const char* txt, size_t size);
      void (*memory_alloc)(const void* ptr, size_t size);
      void (*memory_free)(const void* ptr);
      void (*frame_mark)(const char* name);
      void (*message)(const char* txt, size_t size);
      void (*plot)(const char* name, double value);
    };

    /// one copy per loaded image; null keeps zones inert (static init, plugins before the
    ///  handshake, plugins after profiling was compiled out)
    inline std::atomic<const profiler_backend*> active_backend{ nullptr };

    inline void install_backend(const profiler_backend* backend) {
      active_backend.store(backend, std::memory_order_release);
    }

    /// exe images only: wakes the tracy client in this image and installs the direct table.
    ///  never call from a library that can be unloaded
    void initialize_host_backend();

    /// table the host hands to plugins (records with copied strings); null while profiling
    ///  is off or the host backend is not initialized
    const profiler_backend* plugin_table();

    struct scoped_zone {
      explicit scoped_zone(const source_location* loc) {
        backend = active_backend.load(std::memory_order_acquire);
        if (backend != nullptr) {
          ctx = backend->zone_begin(loc);
        }
      }
      ~scoped_zone() {
        if (backend != nullptr) {
          backend->zone_end(ctx);
        }
      }

      void text(const char* txt, size_t size) {
        if (backend != nullptr) {
          backend->zone_text(ctx, txt, size);
        }
      }

      scoped_zone(scoped_zone&&) = delete;
      scoped_zone(const scoped_zone&) = delete;
      scoped_zone& operator=(scoped_zone&&) = delete;
      scoped_zone& operator=(const scoped_zone&) = delete;

     private:
      const profiler_backend* backend = nullptr;
      zone_ctx ctx{ 0, 0 };
    };

    inline void emit_memory_alloc(const void* ptr, size_t size) {
      if (const profiler_backend* backend = active_backend.load(std::memory_order_acquire); backend != nullptr) {
        backend->memory_alloc(ptr, size);
      }
    }

    inline void emit_memory_free(const void* ptr) {
      if (const profiler_backend* backend = active_backend.load(std::memory_order_acquire); backend != nullptr) {
        backend->memory_free(ptr);
      }
    }

    inline void emit_frame_mark(const char* name) {
      if (const profiler_backend* backend = active_backend.load(std::memory_order_acquire); backend != nullptr) {
        backend->frame_mark(name);
      }
    }

    inline void emit_message(const char* txt, size_t size) {
      if (const profiler_backend* backend = active_backend.load(std::memory_order_acquire); backend != nullptr) {
        backend->message(txt, size);
      }
    }

    inline void emit_plot(const char* name, double value) {
      if (const profiler_backend* backend = active_backend.load(std::memory_order_acquire); backend != nullptr) {
        backend->plot(name, value);
      }
    }

    /// gpu timestamp zones (gl backend) ride their own table so renderer objects stay
    ///  tracy-free: the plugin handshake pulls renderer_backend.obj (on_set) into every
    ///  plugin image, and any tracy reference there would re-embed a client. installed
    ///  exe-side only (other.cpp); zones are render-thread scoped, strictly nested
    struct gpu_profiler_backend {
      void (*context_create)();
      void (*zone_begin)(const source_location* loc);
      void (*zone_end)();
      void (*collect)();
    };

    inline std::atomic<const gpu_profiler_backend*> active_gpu_backend{ nullptr };

    inline void install_gpu_backend(const gpu_profiler_backend* backend) {
      active_gpu_backend.store(backend, std::memory_order_release);
    }

    inline void emit_gpu_context_create() {
      if (const gpu_profiler_backend* backend = active_gpu_backend.load(std::memory_order_acquire); backend != nullptr) {
        backend->context_create();
      }
    }

    inline void emit_gpu_collect() {
      if (const gpu_profiler_backend* backend = active_gpu_backend.load(std::memory_order_acquire); backend != nullptr) {
        backend->collect();
      }
    }

    struct scoped_gpu_zone {
      explicit scoped_gpu_zone(const source_location* loc) {
        backend = active_gpu_backend.load(std::memory_order_acquire);
        if (backend != nullptr) {
          backend->zone_begin(loc);
        }
      }
      ~scoped_gpu_zone() {
        if (backend != nullptr) {
          backend->zone_end();
        }
      }

      scoped_gpu_zone(scoped_gpu_zone&&) = delete;
      scoped_gpu_zone(const scoped_gpu_zone&) = delete;
      scoped_gpu_zone& operator=(scoped_gpu_zone&&) = delete;
      scoped_gpu_zone& operator=(const scoped_gpu_zone&) = delete;

     private:
      const gpu_profiler_backend* backend = nullptr;
    };

  }  // namespace profiling
}  // namespace other

#endif  // OTHER_CORE_PROFILER_BACKEND_HPP

/**
 * \file core/profiler_backend.cpp
 *
 * the only translation unit in the environment that references tracy: exe images pull it
 *  in via initialize_host_backend, unloadable plugins never reference it and therefore
 *  never embed a client (whose static destructor would deadlock FreeLibrary's loader lock
 *  against its own worker-thread joins)
 **/
#include "core/profiler.hpp"
#include "core/profiler_backend.hpp"

#ifdef OTHER_INCLUDE_PROFILING

  #include <cstring>

  #include "tracy/tracy/TracyC.h"

namespace other {
  namespace profiling {

    static_assert(sizeof(source_location) == sizeof(struct ___tracy_source_location_data));
    static_assert(offsetof(source_location, name) == offsetof(struct ___tracy_source_location_data, name));
    static_assert(offsetof(source_location, function) == offsetof(struct ___tracy_source_location_data, function));
    static_assert(offsetof(source_location, file) == offsetof(struct ___tracy_source_location_data, file));
    static_assert(offsetof(source_location, line) == offsetof(struct ___tracy_source_location_data, line));
    static_assert(offsetof(source_location, color) == offsetof(struct ___tracy_source_location_data, color));
    namespace {

      constexpr int32_t kCallstackDepth = TRACY_CALLSTACK;

      /// converted member-wise (no layout cast): tracy's context grows a connectionId
      ///  field under TRACY_ON_DEMAND (0.14) and may grow again
      zone_ctx to_ctx(TracyCZoneCtx ctx) {
        zone_ctx res{ ctx.id, ctx.active, 0 };
#ifdef TRACY_ON_DEMAND
        res.connection_id = ctx.connectionId;
#endif
        return res;
      }

      TracyCZoneCtx from_ctx(zone_ctx ctx) {
        TracyCZoneCtx res;
        res.id = ctx.id;
        res.active = ctx.active;
#ifdef TRACY_ON_DEMAND
        res.connectionId = ctx.connection_id;
#endif
        return res;
      }

      /// direct table (host image): srcloc statics live in the exe for the whole process,
      ///  so tracy can keep the pointer
      zone_ctx direct_zone_begin(const source_location* loc) {
        return to_ctx(___tracy_emit_zone_begin_callstack(
          reinterpret_cast<const struct ___tracy_source_location_data*>(loc), kCallstackDepth, 1));
      }

      /// copying table (plugin images): tracy owns copies of every string, so recorded
      ///  events survive the plugin image unloading
      zone_ctx copying_zone_begin(const source_location* loc) {
        if (___tracy_connected() == 0) {
          return zone_ctx{ 0, 0, 0 };
        }
        const char* name = loc->name != nullptr ? loc->name : "";
        const uint64_t srcloc = ___tracy_alloc_srcloc_name(
          loc->line, loc->file, std::strlen(loc->file), loc->function, std::strlen(loc->function), name, std::strlen(name), loc->color);
        return to_ctx(___tracy_emit_zone_begin_alloc_callstack(srcloc, kCallstackDepth, 1));
      }

      void table_zone_end(zone_ctx ctx) {
        if (ctx.active != 0) {
          ___tracy_emit_zone_end(from_ctx(ctx));
        }
      }

      void table_zone_text(zone_ctx ctx, const char* txt, size_t size) {
        if (ctx.active != 0) {
          ___tracy_emit_zone_text(from_ctx(ctx), txt, size);
        }
      }

      void table_memory_alloc(const void* ptr, size_t size) {
        ___tracy_emit_memory_alloc_callstack(ptr, size, kCallstackDepth);
      }

      void table_memory_free(const void* ptr) {
        ___tracy_emit_memory_free_callstack(ptr, kCallstackDepth);
      }

      void table_frame_mark(const char* name) {
        ___tracy_emit_frame_mark(name);
      }

      /// frame and plot names are cached by pointer viewer-side; a plugin literal dies with
      ///  its image, so the copying table only forwards the anonymous frame mark
      void copying_frame_mark(const char* name) {
        if (name == nullptr) {
          ___tracy_emit_frame_mark(nullptr);
        }
      }

      void table_message(const char* txt, size_t size) {
        ___tracy_emit_logString(TracyMessageSeverityInfo, 0, kCallstackDepth, size, txt);
      }

      void table_plot(const char* name, double value) {
        ___tracy_emit_plot(name, value);
      }

      void copying_plot(const char* name, double value) {
        (void)name;
        (void)value;
      }

      constexpr profiler_backend kDirectTable{
        direct_zone_begin, table_zone_end, table_zone_text, table_memory_alloc,
        table_memory_free, table_frame_mark, table_message, table_plot,
      };

      constexpr profiler_backend kCopyingTable{
        copying_zone_begin, table_zone_end, table_zone_text, table_memory_alloc,
        table_memory_free, copying_frame_mark, table_message, copying_plot,
      };

    }  // namespace

    void initialize_host_backend() {
      install_backend(&kDirectTable);
    }

    const profiler_backend* plugin_table() {
      /// a plugin must never be the image that wakes the client
      return active_backend.load(std::memory_order_acquire) != nullptr ? &kCopyingTable : nullptr;
    }

  }  // namespace profiling
}  // namespace other

#else  // !OTHER_INCLUDE_PROFILING

namespace other {
  namespace profiling {

    void initialize_host_backend() {}

    const profiler_backend* plugin_table() { return nullptr; }

  }  // namespace profiling
}  // namespace other

#endif  // OTHER_INCLUDE_PROFILING

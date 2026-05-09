/**
 * \file tools/environment_console_sink.hpp
 **/
#ifndef OTHERLIB_TOOLS_ENVIRONMENT_CONSOLE_SINK_HPP
#define OTHERLIB_TOOLS_ENVIRONMENT_CONSOLE_SINK_HPP

#include <spdlog/details/null_mutex.h>
#include <spdlog/sinks/base_sink.h>

#include "core/logger.hpp"
#include "core/scope.hpp"
#include "event/event_system.hpp"

namespace other {

  template <typename Mutex>
  class console_sink : public spdlog::sinks::base_sink<Mutex> {
   public:
    console_sink(scope<event_system>& events)
        : events(events) {}
    ~console_sink() override = default;

   protected:
    scope<event_system>& events;

    void sink_it_(const spdlog::details::log_msg& msg) override {
      if (!events) {
        return;
      }

      spdlog::memory_buf_t formatted;

      using namespace spdlog::sinks;
      base_sink<Mutex>::formatter_->format(msg, formatted);
      std::string output_msg = std::string{ formatted };
      switch (msg.level) {
        case spdlog::level::trace: events->trigger_event("console.trace", output_msg); break;
        case spdlog::level::debug: events->trigger_event("console.debug", output_msg); break;
        case spdlog::level::info: events->trigger_event("console.info", output_msg); break;
        case spdlog::level::warn: events->trigger_event("console.warn", output_msg); break;
        case spdlog::level::err: events->trigger_event("console.error", output_msg); break;
        case spdlog::level::critical: events->trigger_event("console.critical", output_msg); break;
        default:
          OTHER_ASSERT(false, "Unhandled log level in console sink: {}", msg.level);
          break;
      }
    }

    void flush_() override {
    }
  };

  using console_sink_mt = console_sink<std::mutex>;
  using console_sink_st = console_sink<spdlog::details::null_mutex>;

}  // namespace other

#endif  // OTHERLIB_TOOLS_ENVIRONMENT_CONSOLE_SINK_HPP
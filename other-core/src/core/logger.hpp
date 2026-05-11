/**
 * \file kernel/logger.hpp
 **/
#ifndef OTHER_CORE_LOGGER_HPP
#define OTHER_CORE_LOGGER_HPP

#include <array>
#include <cstdint>
#include <fstream>
#include <map>
#include <span>

#if __has_include(<stacktrace>)
  #define OTHER_STACKTRACE_AVAILABLE
  #include <stacktrace>
#endif
#include <string>

#include <spdlog/common.h>
#include <spdlog/logger.h>

#include "core/defines.hpp"
#include "core/subsystem.hpp"

namespace other {

  struct log_sink;
  class logger : public subsystem<logger> {
   public:
    logger() = default;
    ~logger() = default;

    static inline uint16_t get_next_sink_id() {
      /// 1, 2 are reserved for stdout and file sinks
      static uint16_t next_id = 3;
      return next_id++;
    }

    natural_t create_logger(const std::string& name, spdlog::level::level_enum level);
    void register_sink(const std::span<const std::string> logs, log_sink* sink);

    void send_log(spdlog::level::level_enum level, natural_t log_idx, const std::string_view msg);

    void set_config(const config_table* config);

   private:
    constexpr static std::string_view kFallbackFile = "other.log";
    constexpr static std::string_view kLogFailureFile = "other-log-failure.log";
    std::unique_ptr<std::ofstream> error_log_file = nullptr;

    std::mutex log_mutex;
    const config_table* current_config_table = nullptr;

    std::map<uint16_t, spdlog::sink_ptr> sinks;
    constexpr static size_t kMaxLoggers = 256;

    natural_t num_loggers = 0;
    struct log {
      std::string name;
      std::shared_ptr<spdlog::logger> logger_ptr = nullptr;
    };
    std::array<log, kMaxLoggers> loggers;

    void log_failure_error(const std::string& message);
  };

  /// figure out why I can't compile when using __VA_OPT__(,) instead of this hack
#define VAR_ARGS(...) , ##__VA_ARGS__

#define LOG(level, log_id, frmt, ...) other::subsystem<other::logger>::get()->send_log(level, log_id, std::format(frmt VAR_ARGS(__VA_ARGS__)))

#define OENV_LOG_TRACE(log_id, fmt, ...) LOG(spdlog::level::trace, log_id, fmt VAR_ARGS(__VA_ARGS__))
#define OENV_LOG_DEBUG(log_id, fmt, ...) LOG(spdlog::level::debug, log_id, fmt VAR_ARGS(__VA_ARGS__))
#define OENV_LOG_INFO(log_id, fmt, ...) LOG(spdlog::level::info, log_id, fmt VAR_ARGS(__VA_ARGS__))
#define OENV_LOG_WARN(log_id, fmt, ...) LOG(spdlog::level::warn, log_id, fmt VAR_ARGS(__VA_ARGS__))
#define OENV_LOG_ERROR(log_id, fmt, ...) LOG(spdlog::level::err, log_id, fmt VAR_ARGS(__VA_ARGS__))
#define OENV_LOG_CRITICAL(log_id, fmt, ...) LOG(spdlog::level::critical, log_id, fmt VAR_ARGS(__VA_ARGS__))

#define CORE_LOG_TRACE(format, ...) OENV_LOG_TRACE(0, format VAR_ARGS(__VA_ARGS__))
#define CORE_LOG_DEBUG(format, ...) OENV_LOG_DEBUG(0, format VAR_ARGS(__VA_ARGS__))
#define CORE_LOG_INFO(format, ...) OENV_LOG_INFO(0, format VAR_ARGS(__VA_ARGS__))
#define CORE_LOG_WARN(format, ...) OENV_LOG_WARN(0, format VAR_ARGS(__VA_ARGS__))
#define CORE_LOG_ERROR(format, ...) OENV_LOG_ERROR(0, format VAR_ARGS(__VA_ARGS__))
#define CORE_LOG_CRITICAL(format, ...) OENV_LOG_CRITICAL(0, format VAR_ARGS(__VA_ARGS__))

  /// \todo add automatic enter/exit function logger structs (raii tracing)

#ifdef OTHER_STACKTRACE_AVAILABLE
  #include <sstream>
  #include <stacktrace>
  #define OTHER_STACKTRACE (std::stringstream{} << std::stacktrace::current() << "\n").str()
#else
  #define OTHER_STACKTRACE "Stacktrace not available (no <stacktrace> support)"
#endif

#define OTHER_CRITICAL_FAILURE(format, ...)                                                                    \
  do {                                                                                                         \
    CORE_LOG_CRITICAL("Critical failure!\nstacktrace =\n{}\n" format, OTHER_STACKTRACE VAR_ARGS(__VA_ARGS__)); \
    OTHER_ABORT();                                                                                             \
  } while (0)

#define OTHER_ASSERT(condition, format, ...)       \
  do {                                             \
    if ((condition)) {                             \
    } else {                                       \
      OTHER_CRITICAL_FAILURE(format, __VA_ARGS__); \
    }                                              \
  } while (0)

#define OTHER_UNIMPLEMENTED_FUNCTION() \
  OTHER_ASSERT(false, "Calling unimplemented function: {} at {}", std::source_location::current().function_name(), std::source_location::current().line())

#define OTHER_UNIMPLEMENTED_FUNCTION_RETURN(expr)                                                                                                           \
  OTHER_ASSERT(false, "Calling unimplemented function: {} at {}", std::source_location::current().function_name(), std::source_location::current().line()); \
  return expr;

}  // namespace other

OTHER_SUBSYSTEM(other::logger);

#endif  // OTHER_CORE_LOGGER_HPP
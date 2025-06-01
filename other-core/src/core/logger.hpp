/**
 * \file kernel/logger.hpp
 **/
#ifndef OTHER_CORE_LOGGER_HPP
#define OTHER_CORE_LOGGER_HPP

#include <cstdint>
#include <fstream>
#include <span>
#include <stacktrace>
#include <string>

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_sinks.h>
#include <spdlog/sinks/wincolor_sink.h>
#include <spdlog/spdlog.h>

#include "core/defines.hpp"
#include "core/subsystem.hpp"

namespace other {

  using sink_fn = spdlog::sink_ptr (*)();
  struct log_sink {
    uint16_t id;
    std::string sink_name;
    std::string sink_pattern;
    spdlog::level::level_enum level;

    sink_fn sink_factory = nullptr;
  };

  class logger : public subsystem<logger> {
   public:
    logger() = default;
    ~logger() = default;

    void create_logger(const std::string& name, spdlog::level::level_enum level);
    void register_sink(const std::span<const std::string> logs, const log_sink& sink);

    void send_log(spdlog::level::level_enum level, const std::string_view log_name, const std::string_view msg);

   private:
    constexpr static std::string_view kFallbackFile = "other.log";
    constexpr static std::string_view kLogFailureFile = "other-log-failure.log";
    std::unique_ptr<std::ofstream> error_log_file = nullptr;

    std::map<uint16_t, spdlog::sink_ptr> sinks;
    std::map<natural_t, std::shared_ptr<spdlog::logger>> loggers;

    void log_failure_error(const std::string& message);
  };

  OTHER_SUBSYSTEM(logger);

#define LOG(level, logger_name, frmt, ...) \
  other::subsystem<other::logger>::get()->send_log(level, logger_name, std::format(frmt, ##__VA_ARGS__))

#define LOG_TRACE(logger_name, fmt, ...) LOG(spdlog::level::trace, logger_name, fmt, __VA_ARGS__)
#define LOG_DEBUG(logger_name, fmt, ...) LOG(spdlog::level::debug, logger_name, fmt, __VA_ARGS__)
#define LOG_INFO(logger_name, fmt, ...) LOG(spdlog::level::info, logger_name, fmt, __VA_ARGS__)
#define LOG_WARN(logger_name, fmt, ...) LOG(spdlog::level::warn, logger_name, fmt, __VA_ARGS__)
#define LOG_ERROR(logger_name, fmt, ...) LOG(spdlog::level::err, logger_name, fmt, __VA_ARGS__)
#define LOG_CRITICAL(logger_name, fmt, ...) LOG(spdlog::level::critical, logger_name, fmt, __VA_ARGS__)

#define CORE_LOG_TRACE(format, ...) LOG_TRACE("other-core-log", format, __VA_ARGS__)
#define CORE_LOG_DEBUG(format, ...) LOG_DEBUG("other-core-log", format, __VA_ARGS__)
#define CORE_LOG_INFO(format, ...) LOG_INFO("other-core-log", format, __VA_ARGS__)
#define CORE_LOG_WARN(format, ...) LOG_WARN("other-core-log", format, __VA_ARGS__)
#define CORE_LOG_ERROR(format, ...) LOG_ERROR("other-core-log", format, __VA_ARGS__)
#define CORE_LOG_CRITICAL(format, ...) LOG_CRITICAL("other-core-log", format, __VA_ARGS__)
  /// \todo add automatic enter/exit function logger structs (raii tracing)

#define GET_STACKTRACE (std::stringstream{} << std::stacktrace::current() << "\n").str()
#ifndef OTHER_TEST_ENVIRONMENT
  #define OTHER_ABORT std::terminate()
#else
  #define OTHER_ABORT throw std::runtime_error("Critical failure in test environment, aborting.");
#endif

#define OTHER_CRITICAL_FAILURE(format, ...)                                                           \
  do {                                                                                                \
    CORE_LOG_CRITICAL("Critical failure!\nstacktrace =\n{}\n" format, GET_STACKTRACE, ##__VA_ARGS__); \
    OTHER_ABORT;                                                                                      \
  } while (0)

#define OTHER_ASSERT(condition, format, ...)       \
  do {                                             \
    if ((condition)) {                             \
    } else {                                       \
      OTHER_CRITICAL_FAILURE(format, __VA_ARGS__); \
    }                                              \
  } while (0)

}  // namespace other

#endif  // OTHER_CORE_LOGGER_HPP
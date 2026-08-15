/**
 * \file core/logger.cpp
 **/
#include "core/logger.hpp"

#include <cstdio>
#include <source_location>

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_sinks.h>
#include <spdlog/sinks/wincolor_sink.h>
#include <spdlog/spdlog.h>

#include "core/config_table.hpp"
#include "core/fnv.hpp"
#include "core/logger_sinks.hpp"
#include "core/profiler.hpp"

#include "spdlog/sinks/basic_file_sink.h"

namespace other {

  natural_t logger::create_logger(const std::string& name, spdlog::level::level_enum level) {
    PROFILE_SECTION("logger::create-logger");

    std::unique_lock lock(log_mutex);
    std::shared_ptr<spdlog::logger> logger = std::make_shared<spdlog::logger>(name);
    logger->set_level(level);
    logger->flush_on(level);

    natural_t id = num_loggers++;
    if (id >= kMaxLoggers) {
      log_failure_error_unlocked("Maximum number of loggers reached.");
      return static_cast<natural_t>(-1);
    }
    loggers[id] = log{
      .name = name,
      .logger_ptr = logger,
    };
    return id;
  }

  void logger::register_sink(const std::span<const std::string> logs, log_sink* sink) {
    PROFILE_SECTION("logger::register-sink");

    if (sink == nullptr) {
      log_failure_error("Sink is null.");
      return;
    }

    if (sink->sink_factory == nullptr) {
      log_failure_error(std::format("Sink factory for {} is null.", sink->sink_name));
      return;
    }

    if (sink->id == 0) {
      log_failure_error(std::format("Sink ID for {} is 0.", sink->sink_name));
      return;
    }

    if (sink->sink_name.empty()) {
      log_failure_error(std::format("Sink name for {} is empty.", sink->sink_name));
      return;
    }

    if (sink->sink_pattern.empty()) {
      log_failure_error(std::format("Sink pattern for {} is empty.", sink->sink_name));
      return;
    }

    auto sink_ptr = sink->sink_factory(*current_config_table);
    if (sink_ptr == nullptr) {
      log_failure_error(std::format("Failed to create sink for {}.", sink->sink_name));
      return;
    }

    std::unique_lock lock(log_mutex);
    {
      auto [itr, inserted] = sinks.insert({ sink->id, std::move(sink_ptr) });
      if (!inserted) {
        log_failure_error_unlocked(std::format("Sink with ID {} ({}) already exists.", sink->id, sink->sink_name));
        return;
      }

      auto& sink_ptr = itr->second;
      sink_ptr->set_pattern(sink->sink_pattern);
      sink_ptr->set_level(sink->level);
      for (const auto& log : logs) {
        if (log.empty()) {
          continue;
        }

        for (natural_t i = 0; i < num_loggers; ++i) {
          if (loggers[i].name == log) {
            loggers[i].logger_ptr->sinks().push_back(sink_ptr);
          }
        }
      }
    }
  }

  void logger::send_log(spdlog::level::level_enum level, natural_t log_id, const std::string_view msg) {
    if (log_id >= num_loggers) {
      /// contract failure reported raw: OTHER_ASSERT logs back through this logger and would
      ///  recurse (stderr direct, same reasoning as report_unhandled_seh)
      const std::string failure = std::format(
        "Critical failure! Invalid log ID: {} (num loggers: {})\nstacktrace =\n{}\ndropped log:\n{}\n",
        log_id, num_loggers, OTHER_STACKTRACE, msg);
      std::fputs(failure.c_str(), stderr);
      OTHER_ABORT();
    }

    auto& log_entry = loggers[log_id];
    if (log_entry.logger_ptr->level() == spdlog::level::off) {
      return;
    }

    switch (level) {
      case spdlog::level::trace:
        log_entry.logger_ptr->trace(msg);
        break;
      case spdlog::level::debug:
        log_entry.logger_ptr->debug(msg);
        break;
      case spdlog::level::info:
        log_entry.logger_ptr->info(msg);
        break;
      case spdlog::level::warn:
        log_entry.logger_ptr->warn(msg);
        break;
      case spdlog::level::err:
        log_entry.logger_ptr->error(msg);
        break;
      case spdlog::level::critical:
        log_entry.logger_ptr->critical(msg);
        break;
      default:
        log_failure_error(std::format("Logger {} has invalid level. Dropped Log :\n{}", log_entry.name, msg));
    }
  }

  void logger::set_config(const config_table* config) {
    this->current_config_table = config;
  }

  void logger::send_log_guarded(spdlog::level::level_enum level, natural_t log_id, const std::string_view msg) {
    logger* log = subsystem<logger>::try_get();
    if (log == nullptr) {
      dropped_log_error(level, msg);
      return;
    }
    log->send_log(level, log_id, msg);
  }

  void logger::dropped_log_error(spdlog::level::level_enum level, const std::string_view msg) {
    /// leaked mutex: drops can arrive during static destruction, after function-locals are gone
    static std::mutex& dropped_mutex = *(new std::mutex());

    auto now = std::chrono::system_clock::now();
    std::time_t now_time = std::chrono::system_clock::to_time_t(now);
    std::tm now_tm = *std::localtime(&now_time);

    std::stringstream time_stream;
    time_stream << std::put_time(&now_tm, "%Y-%m-%d %H:%M:%S");

    std::lock_guard lock(dropped_mutex);
    std::ofstream file(kLogFailureFile.data(), std::ios::app);
    file << "[" << time_stream.str() << "] DROPPED LOG (logger subsystem inactive): " << msg << std::endl;

    /// assert failure paths log at critical then abort — that report must still reach the console
    if (level == spdlog::level::critical) {
      const std::string err = std::format("DROPPED CRITICAL LOG (logger subsystem inactive): {}\n", msg);
      std::fputs(err.c_str(), stderr);
    }
  }

  void logger::log_failure_error(const std::string& message) {
    std::unique_lock lock(log_mutex);
    log_failure_error_unlocked(message);
  }

  void logger::log_failure_error_unlocked(const std::string& message) {
    if (error_log_file == nullptr) {
      error_log_file = std::make_unique<std::ofstream>(kLogFailureFile.data(), std::ios::app);
    }
    /// get time and date
    auto now = std::chrono::system_clock::now();
    std::time_t now_time = std::chrono::system_clock::to_time_t(now);
    std::tm now_tm = *std::localtime(&now_time);

    std::stringstream time_stream;
    time_stream << std::put_time(&now_tm, "%Y-%m-%d %H:%M:%S");

    std::string time_str = time_stream.str();
    *error_log_file << "[" << time_str << "] "
                    << "LOG FAILURE ERROR: " << message << std::endl;
    *error_log_file << message << std::endl;
    error_log_file = nullptr;
  }

}  // namespace other
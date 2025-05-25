/**
 * \file core/logger.cpp
 **/
#include "core/logger.hpp"

#include "core/fnv.hpp"

namespace other {

  void logger::create_logger(const std::string& name, spdlog::level::level_enum level) {
    // PROFILE_SECTION("Logger--CreateLogger");
    std::unique_ptr<spdlog::logger> logger = std::make_unique<spdlog::logger>(name);
    logger->set_level(level);
    logger->flush_on(level);

    uint64_t id = FNV(name);
    auto [itr, inserted] = loggers.insert({ id, std::move(logger) });
    if (!inserted) {
      log_failure_error(std::format("Logger with name {} already exists.", name));
      logger = nullptr;
    }
  }

  void logger::register_sink(const std::span<const std::string> logs, const log_sink& sink) {
    // PROFILE_SECTION("Logger--RegisterSink");

    if (sink.sink_factory == nullptr) {
      log_failure_error(std::format("Sink factory for {} is null.", sink.sink_name));
      return;
    }

    if (sink.id == 0) {
      log_failure_error(std::format("Sink ID for {} is 0.", sink.sink_name));
      return;
    }

    if (sink.sink_name.empty()) {
      log_failure_error(std::format("Sink name for {} is empty.", sink.sink_name));
      return;
    }

    if (sink.sink_pattern.empty()) {
      log_failure_error(std::format("Sink pattern for {} is empty.", sink.sink_name));
      return;
    }

    auto sink_ptr = sink.sink_factory();
    if (sink_ptr == nullptr) {
      log_failure_error(std::format("Failed to create sink for {}.", sink.sink_name));
      return;
    }

    auto [itr, inserted] = sinks.insert({ sink.id, std::move(sink_ptr) });
    if (!inserted) {
      log_failure_error(std::format("Sink with ID {} ({}) already exists.", sink.id, sink.sink_name));
      return;
    }
    {
      auto& sink_ptr = itr->second;
      sink_ptr->set_pattern(sink.sink_pattern);
      sink_ptr->set_level(sink.level);
      for (const auto& log : logs) {
        auto logger_itr = loggers.find(FNV(log));

        if (logger_itr != loggers.end()) {
          logger_itr->second->sinks().push_back(sink_ptr);
        } else {
          log_failure_error(std::format("Logger {} not found for sink {}.", log, sink.sink_name));
        }
      }
    }
  }

  void logger::send_log(spdlog::level::level_enum level, const std::string_view log_name, const std::string_view msg) {
    auto logger_itr = loggers.find(FNV(log_name));
    if (logger_itr == loggers.end()) {
      log_failure_error(std::format("Logger {} not found. Dropped Log :\n{}", log_name, msg));
      return;
    }

    auto& logger = logger_itr->second;
    if (logger == nullptr) {
      log_failure_error(std::format("Logger {} is null. Dropped Log :\n{}", log_name, msg));
      return;
    }

    if (logger->level() == spdlog::level::off) {
      return;
    }

    switch (level) {
      case spdlog::level::trace:
        logger->trace(msg);
        break;
      case spdlog::level::debug:
        logger->debug(msg);
        break;
      case spdlog::level::info:
        logger->info(msg);
        break;
      case spdlog::level::warn:
        logger->warn(msg);
        break;
      case spdlog::level::err:
        logger->error(msg);
        break;
      case spdlog::level::critical:
        logger->critical(msg);
        break;
      default:
        log_failure_error(std::format("Logger {} has invalid level. Dropped Log :\n{}", log_name, msg));
    }
  }

  void logger::log_failure_error(const std::string& message) {
    if (error_log_file == nullptr) {
      error_log_file = std::make_unique<std::ofstream>(kLogFailureFile.data(), std::ios::app);
    }
    *error_log_file << message << std::endl;
  }

}  // namespace other
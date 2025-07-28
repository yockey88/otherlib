/**
 * \file bindings/native_logger.cpp
 **/
#include "bindings/native_logger.hpp"

#include "core/logger.hpp"

namespace other {
  namespace bindings {

    void native_log_message(logger* logger, native_string message, int32_t level) {
      switch ((native_log_level)level) {
        case TRACE:
          logger->send_log(spdlog::level::trace, "other-core-log", (std::string)message);
          break;
        case DEBUG:
          logger->send_log(spdlog::level::debug, "other-core-log", (std::string)message);
          break;
        case INFO:
          logger->send_log(spdlog::level::info, "other-core-log", (std::string)message);
          break;
        case WARNING:
          logger->send_log(spdlog::level::warn, "other-core-log", (std::string)message);
          break;
        case ERR:
          logger->send_log(spdlog::level::err, "other-core-log", (std::string)message);
          break;
        case CRITICAL:
          logger->send_log(spdlog::level::critical, "other-core-log", (std::string)message);
          break;
        default:
          CORE_LOG_ERROR("Unknown log level: {}", level);
          break;
      }
    }

  }  // namespace bindings
}  // namespace other
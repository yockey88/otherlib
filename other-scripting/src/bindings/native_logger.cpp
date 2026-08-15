/**
 * \file bindings/native_logger.cpp
 **/
#include "bindings/native_logger.hpp"

#include "core/logger.hpp"

namespace other {
  namespace bindings {

    /// managed code holds this handle process-long, so it can outlive the logger subsystem —
    ///  route through the guarded send (liveness-checked) instead of the cached pointer
    void native_log_message([[maybe_unused]] logger* cached_logger, native_string message, int32_t level) {
      switch ((native_log_level)level) {
        case TRACE:
          logger::send_log_guarded(spdlog::level::trace, 0, (std::string)message);
          break;
        case DEBUG:
          logger::send_log_guarded(spdlog::level::debug, 0, (std::string)message);
          break;
        case INFO:
          logger::send_log_guarded(spdlog::level::info, 0, (std::string)message);
          break;
        case WARNING:
          logger::send_log_guarded(spdlog::level::warn, 0, (std::string)message);
          break;
        case ERR:
          logger::send_log_guarded(spdlog::level::err, 0, (std::string)message);
          break;
        case CRITICAL:
          logger::send_log_guarded(spdlog::level::critical, 0, (std::string)message);
          break;
        default:
          CORE_LOG_ERROR("Unknown log level: {}", level);
          break;
      }
    }

  }  // namespace bindings
}  // namespace other
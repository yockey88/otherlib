/**
 * \file kernel/logger.hpp
 **/
#ifndef OTHERENV_KERNEL_LOGGER_HPP
#define OTHERENV_KERNEL_LOGGER_HPP

#include "spdlog/spdlog.h"

namespace other {

  using SinkFn = std::function<spdlog::sink_ptr()>;
  struct Sink {
    std::string sink_name;
    std::string sink_pattern;
    spdlog::level::level_enum level;
  };

  struct LoggerTargetData {
    Sink sink;
    SinkFn sink_factory = nullptr;
  };

}  // namespace other

#endif  // OTHERENV_KERNEL_LOGGER_HPP
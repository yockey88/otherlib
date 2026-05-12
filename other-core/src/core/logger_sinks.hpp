/**
 * \file core/logger_sinks.hpp
 **/
#ifndef OTHER_CORE_LOGGER_SINKS_HPP
#define OTHER_CORE_LOGGER_SINKS_HPP

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/wincolor_sink.h>
#include <spdlog/spdlog.h>

namespace other {

  class config_table;

  using sink_fn = std::function<spdlog::sink_ptr(const config_table& config)>;
  struct log_sink {
    uint16_t id;
    std::string sink_name;
    std::string sink_pattern;
    spdlog::level::level_enum level;

    sink_fn sink_factory = nullptr;
  };

}  // namespace other

#endif  // OTHER_CORE_LOGGER_SINKS_HPP
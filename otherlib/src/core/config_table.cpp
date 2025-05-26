/**
 * \file core/config_table.cpp
 **/
#include "core/config_table.hpp"

#include <iostream>
#include <string>

#include "core/logger.hpp"

namespace other {

  opt<config_table> parse_raw_config(const std::string_view filename) {
    toml::table table;
    std::string driver = "";
    std::string rendering = "";
    int32_t log_level = 0;

    try {
      table = toml::parse_file(filename);

      toml::node_view driver_path = table.at_path("application.driver");
      driver = driver_path.as_string() == nullptr ? "" : driver_path.as_string()->get();

      toml::node_view rendering_backend = table.at_path("rendering.rendering-api");
      rendering = rendering_backend.as_string() == nullptr ? "" : rendering_backend.as_string()->get();

      toml::node_view log_level_node = table.at_path("application.core-log-level");
      if (log_level_node.is_integer()) {
        log_level = log_level_node.as_integer()->get();
      } else if (log_level_node.is_string()) {
        std::string level_str = log_level_node.as_string()->get();
        switch (FNV(level_str)) {
          case FNV("trace"):
            log_level = 0;
            break;
          case FNV("debug"):
            log_level = 1;
            break;
          case FNV("info"):
            log_level = 2;
            break;
          case FNV("warn"):
            log_level = 3;
            break;
          case FNV("error"):
            log_level = 4;
            break;
          case FNV("critical"):
            log_level = 5;
            break;
          default:
            log_level = 2;  // Default to info
        }
      } else {
        log_level = 2;
      }

    } catch (const toml::parse_error& err) {
      std::println(std::cerr, "Failed to parse configuration file '{}': {}", filename, err.description());
      return std::nullopt;
    }

    config_table config;
    config.table = std::move(table);

    config.valid = true;
    if (!driver.empty()) {
      config.dynamic_driver_rel_path = driver;
    }
    if (!rendering.empty()) {
      config.rendering_backend = rendering;
    }
    config.core_log_level = log_level;

    return config;
  }

  config_table config_table::load(const std::string_view filename) {
    return std::move(parse_raw_config(filename).value_or(config_table{}));
  }

  value config_table::get_value(const std::string_view section, const std::string_view key) const {
    return value();
  }

}  // namespace other
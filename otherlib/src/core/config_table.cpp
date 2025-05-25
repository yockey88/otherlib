/**
 * \file core/config_table.cpp
 **/
#include "core/config_table.hpp"

#include "core/logger.hpp"

namespace other {

  opt<config_table> parse_raw_config(const std::string_view filename) {
    toml::table table;
    std::string driver = "";

    try {
      table = toml::parse_file(filename);

      toml::node_view driver_path = table.at_path("application.driver");
      driver = driver_path.as_string() == nullptr ? "" : driver_path.as_string()->get();
    } catch (const toml::parse_error& err) {
      CORE_LOG_ERROR("Failed to parse config file '{}': {}", filename, err.description());
      return std::nullopt;
    }

    config_table config;
    config.table = std::move(table);

    config.valid = true;
    if (!driver.empty()) {
      config.dynamic_driver_rel_path = driver;
    }

    return config;
  }

  config_table config_table::load(const std::string_view filename) {
    return std::move(parse_raw_config(filename).value_or(config_table{}));
  }

  value config_table::get_value(const std::string_view section, const std::string_view key) const {
    return value();
  }

}  // namespace other
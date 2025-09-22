/**
 * \file core/config_table.cpp
 **/
#include "core/config_table.hpp"

#include <iostream>
#include <print>
#include <string>

#include "core/fnv.hpp"
#include "core/logger.hpp"

namespace other {

  opt<config_table> parse_raw_config(const std::string_view filename) {
    if (filename.empty()) {
      config_table c{};
      c.valid = true;
      return c;
    }
    std::println(std::cout, "Loading Other Environment from configuration: {}", filename);

    toml::table table;
    opt<toml::table> project_table;

    std::string driver = "";
    int32_t log_level = 0;

    bool open_terminal = false;

    std::string rendering = "";
    glm::uvec2 window_size = { 1280, 720 };

    try {
      table = toml::parse_file(filename);

      toml::node_view driver_path = table.at_path("application.driver");
      driver = driver_path.as_string() == nullptr ? "" : driver_path.as_string()->get();

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

      toml::node_view environment = table.at_path("environment");
      if (environment.is_table()) {
        toml::node_view open_terminal_node = environment.at_path("terminal");
        if ((open_terminal_node.is_boolean() && *open_terminal_node.as_boolean()) ||
            (open_terminal_node.is_string() && (open_terminal_node.as_string()->get() == "true" || open_terminal_node.as_string()->get() == "on"))) {
          open_terminal = true;
        } else if ((open_terminal_node.is_boolean() && !*open_terminal_node.as_boolean()) ||
                   (open_terminal_node.is_string() && (open_terminal_node.as_string()->get() == "false" || open_terminal_node.as_string()->get() == "off"))) {
          open_terminal = false;
        } else {
          std::println(std::cerr, "Invalid value for 'environment.terminal', not opening.");
        }
      }
      if (log_level == 0) {
        std::print("Terminal open: {}\n", open_terminal);
      }

      toml::node_view rendering_backend = table.at_path("rendering.rendering-backend");
      rendering = rendering_backend.as_string() == nullptr ? "" : rendering_backend.as_string()->get();
      if (log_level == 0) {
        std::print("Rendering backend: '{}'\n", rendering);
      }

      toml::node_view window_size_node = table.at_path("rendering.window-size");
      if (window_size_node.is_table()) {
        toml::node_view width_node = window_size_node.at_path("width");
        toml::node_view height_node = window_size_node.at_path("height");
        if (width_node.is_integer() && height_node.is_integer()) {
          window_size = { width_node.as_integer()->get(), height_node.as_integer()->get() };
        }

        if (log_level == 0) {
          std::print("Window size: {}x{}\n", window_size.x, window_size.y);
        }
      }

      {
        toml::node_view ptable = table.at_path("project");
        if (ptable.is_table()) {
          if (auto* p = ptable.as_table(); p != nullptr) {
            project_table.emplace(std::move(*p));
          }

          if (log_level == 0) {
            std::print("Loaded project table with {} entries.\n", project_table->size());
          }
        }
      }

    } catch (const toml::parse_error& err) {
      std::println(std::cerr, "Failed to parse configuration file '{}' caught a toml-parse-error: {}", filename, err.description());
      return std::nullopt;
    }

    config_table config;
    config.table = std::move(table);

    config.valid = true;
    if (!driver.empty()) {
      config.dynamic_driver_rel_path = driver;
    }
    config.core_log_level = log_level;

    config.open_terminal = open_terminal;

    if (!rendering.empty()) {
      config.rendering_backend = rendering;
    }
    config.window_size = window_size;

    return config;
  }

  config_table config_table::load(const std::string_view filename) {
    return std::move(parse_raw_config(filename).value_or(config_table{}));
  }

  value config_table::get_project_value(const std::string_view section, const std::string_view key) const {
    return value();
  }

  toml::table& config_table::get_project_table() {
    if (project_table.has_value()) {
      return project_table.value();
    }

    static toml::table empty_table;
    return empty_table;
  }

}  // namespace other
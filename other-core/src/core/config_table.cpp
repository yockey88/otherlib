/**
 * \file core/config_table.cpp
 **/
#include "core/config_table.hpp"

#include <iostream>
#include <print>
#include <string>

#include "core/fnv.hpp"
#include "core/logger.hpp"
#include "core/profiler.hpp"

namespace other {

  opt<config_table> parse_string_config(const std::string_view contents) {
    PROFILE_SECTION("config_table::parse_string_config");
    if (contents.empty()) {
      config_table c{};
      c.valid = true;
      return c;
    }

    std::string driver = "";
    int32_t log_level = 0;
    int32_t file_log_level = 0;
    std::string core_log_file = "logs/other_env.log";

    bool open_terminal = false;

    std::string rendering = "";

#if 0
      std::println(std::cout, "Parsing configuration file...");
      std::println(std::cout, "----------------------------------------");
      std::println(std::cout, "{}", contents);
      std::println(std::cout, "----------------------------------------");
#endif

    config_table config = {};
    config.table = toml::parse(contents);

    toml::node_view driver_path = config.table.at_path("application.driver");
    driver = driver_path.as_string() == nullptr ? "" : driver_path.as_string()->get();

    toml::node_view dotnet_modules = config.table.at_path("scripting.dotnet-modules");
    if (dotnet_modules.is_array()) {
      dotnet_modules.as_array()->for_each([&](auto&& elem) {
        if (elem.is_string()) {
          std::string module_path = elem.as_string()->get();
          std::println(" - .NET module to load from config: {}", module_path);
        }
      });
    } else {
      std::println("No .NET modules specified in configuration.");
    }

    toml::node_view log_level_node = config.table.at_path("application.core-log-level");
    toml::node_view file_log_level_node = config.table.at_path("application.file-log-level");
    toml::node_view log_file_node = config.table.at_path("application.core-log-file");

    auto get_log_level = [](toml::node_view<toml::node> node, int32_t default_level) -> int32_t {
      if (node.is_integer()) {
        return node.as_integer()->get();
      } else if (node.is_string()) {
        std::string level_str = node.as_string()->get();
        switch (FNV(level_str)) {
          case FNV("trace"): return 0;
          case FNV("debug"): return 1;
          case FNV("info"): return 2;
          case FNV("warn"): return 3;
          case FNV("error"): return 4;
          case FNV("critical"): return 5;
          default:
            return default_level;  // Default to provided default level
        }
      } else {
        return default_level;
      }
    };
    log_level = get_log_level(log_level_node, 2);            // default = info
    file_log_level = get_log_level(file_log_level_node, 0);  // default = trace

    std::println("using log level: {}", log_level);
    std::println("using file log level: {}", file_log_level);

    if (log_file_node.is_string()) {
      core_log_file = log_file_node.as_string()->get();
    }

    toml::node_view environment = config.table.at_path("environment");
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

    toml::node_view force_no_window = config.table.at_path("rendering.force-no-window");
    if (force_no_window.is_boolean()) {
      config.force_no_window = force_no_window.as_boolean();
    }

    toml::node_view rendering_backend = config.table.at_path("rendering.backend");
    rendering = rendering_backend.as_string() == nullptr ? "" : rendering_backend.as_string()->get();

    toml::node_view window_size_node = config.table.at_path("rendering.window-size");
    if (window_size_node.is_table()) {
      toml::node_view width_node = window_size_node.at_path("width");
      toml::node_view height_node = window_size_node.at_path("height");
      if (width_node.is_integer() && height_node.is_integer()) {
        config.window_size = { width_node.as_integer()->get(), height_node.as_integer()->get() };
      }

      if (log_level == 0) {
        std::print("Terminal open: {}\n", open_terminal);
        std::print("Rendering backend: '{}'\n", rendering);
        std::print("Window size: {}x{}\n", config.window_size.x, config.window_size.y);
      }
    }

    toml::node_view project_file = config.table.at_path("application.project-file");
    if (project_file.is_string()) {
      filepath path = filepath(project_file.as_string()->get());
      if (path.empty()) {
        std::println(std::cerr, "Invalid value for 'application.project-file', empty path provided.");
      } else if (!std::filesystem::exists(path)) {
        std::println(std::cerr, "Invalid value for 'application.project-file', file does not exist: {}", path.string());
      } else {
        config.project_file = path;
        CORE_LOG_INFO("Using project file: {}", path.string());
      }
    }

    if (!driver.empty()) {
      config.dynamic_driver_rel_path = driver;
    }
    if (!rendering.empty()) {
      config.rendering_backend = rendering;
    }

    config.file_log_level = file_log_level;
    config.core_log_level = log_level;
    config.core_log_file = core_log_file;
    config.open_terminal = open_terminal;

    config.valid = true;
    return config;
  }

  opt<config_table> parse_raw_config(const std::string_view filename) {
    PROFILE_SECTION("config_table::parse_raw_config");
    if (filename.empty()) {
      config_table c{};
      c.valid = true;
      return c;
    }
    std::println(std::cout, "Loading Other Environment from configuration: {}", std::filesystem::absolute(filepath(filename)).string());

    try {
      std::string contents;
      {
        std::stringstream ss;
        std::ifstream file(std::string{ filename });
        if (!file.is_open()) {
          std::println(std::cerr, "Failed to open configuration file: '{}'", filename);
          return std::nullopt;
        }
        ss << file.rdbuf();
        contents = ss.str();
      }
      return parse_string_config(contents);
    } catch (const std::exception& e) {
      std::println(std::cerr, "Failed to read configuration file '{}' with error: {}", filename, e.what());
      return std::nullopt;
    }
  }

  config_table config_table::load(const std::string_view filename) {
    return parse_raw_config(filename).value_or(config_table{});
  }

  config_table config_table::load_from_source(const std::string_view text) {
    return parse_string_config(text).value_or(config_table{});
  }

  value config_table::get_project_value(const std::string_view toml_path) const {
    toml::node_view node = table.at_path(std::format("project.{}", toml_path));
    if (!node) {
      CORE_LOG_WARN("Project config key '{}' not found, returning default value.", toml_path);
      return value();
    } else {
      CORE_LOG_TRACE("Found project config key '{}'", toml_path);
    }

    /// \todo
    return value();
  }

  toml::table& config_table::get_project_table() {
    return table;
  }

  const toml::table& config_table::get_project_table() const {
    return table;
  }

  std::string config_table::dump_table_string() const {
    std::ostringstream oss;
    oss << table;
    return oss.str();
  }

  const toml::table* config_table::get_subtable(const std::string_view toml_path) const {
    toml::node_view node = table.at_path(toml_path);
    if (!node) {
      CORE_LOG_WARN("Config subtable '{}' not found.", toml_path);
      return nullptr;
    } else if (!node.is_table()) {
      CORE_LOG_WARN("Config subtable '{}' is not a table.", toml_path);
      return nullptr;
    } else {
      return node.as_table();
    }
  }

}  // namespace other
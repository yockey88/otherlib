/**
 * \file core/config_table.cpp
 **/
#include "core/config_table.hpp"

#include <fstream>
#include <iostream>
#include <print>
#include <sstream>
#include <string>

#include <toml++/toml.h>

#include "core/fnv.hpp"
#include "core/logger.hpp"
#include "core/profiler.hpp"

namespace other {

  namespace detail {

    toml_table_box::toml_table_box()
        : ptr(std::make_unique<toml::table>()) {}

    toml_table_box::toml_table_box(const toml_table_box& other)
        : ptr(std::make_unique<toml::table>(*other.ptr)) {}

    toml_table_box& toml_table_box::operator=(const toml_table_box& other) {
      PROFILE_SECTION("toml_table_box::operator=");
      if (this != &other) {
        ptr = std::make_unique<toml::table>(*other.ptr);
      }
      return *this;
    }

    toml_table_box::toml_table_box(toml_table_box&& other) noexcept = default;
    toml_table_box& toml_table_box::operator=(toml_table_box&& other) noexcept = default;

    toml_table_box::~toml_table_box() = default;

  }  // namespace detail

  namespace {

    template <typename T>
    bool check_type(auto n, const std::string_view toml_path) {
      if (!n) {
        CORE_LOG_ERROR("Config key '{}' not found.", toml_path);
        return false;
      }

      if constexpr (std::is_same_v<T, toml::table>) {
        return n.is_table();
      } else if constexpr (std::is_same_v<T, std::string>) {
        return n.is_string();
      } else if constexpr (std::is_integral_v<T> && !std::is_same_v<T, bool>) {
        return n.is_integer();
      } else if constexpr (std::is_same_v<T, float> || std::is_same_v<T, double>) {
        return n.is_floating_point();
      } else if constexpr (std::is_same_v<T, bool>) {
        return n.is_boolean();
      } else if constexpr (is_container<T> && !is_stringlike_type<T>) {
        return n.is_array();
      } else {
        static_assert(false, "Unsupported type for config value.");
      }
    }

    template <typename T>
    T return_node(auto n, const std::string_view toml_path) {
      if constexpr (std::is_same_v<T, toml::table>) {
        return *n.as_table();
      } else if constexpr (std::is_same_v<T, std::string>) {
        /// here we replace ${x} with environment variables/necessary
        /// replacements
        std::string v = n.as_string()->get();
        return perform_tag_replacement(v);
      } else if constexpr (std::is_integral_v<T> && !std::is_same_v<T, bool>) {
        int64_t v = n.as_integer()->get();
        return static_cast<T>(v);
      } else if constexpr (std::is_same_v<T, float> || std::is_same_v<T, double>) {
        double v = n.as_floating_point()->get();
        return static_cast<T>(v);
      } else if constexpr (std::is_same_v<T, bool>) {
        bool v = n.as_boolean()->get();
        return v;
      } else if constexpr (is_container<T> && !is_stringlike_type<T>) {
        using vtype = typename T::value_type;

        T result = {};
        const toml::array* array_node = n.as_array();
        if (array_node == nullptr) {
          CORE_LOG_WARN("Config key '{}' is not an array, returning default value.", toml_path);
          return result;
        }

        CORE_LOG_TRACE("Parsing config array for key '{}' ({} items)", toml_path, array_node->size());
        array_node->for_each([&](auto&& elem) {
          result.push_back(return_node<vtype>(elem, toml_path));
        });
        return result;
      } else {
        static_assert(false, "Unsupported type for config value.");
      }
    }

  }  // namespace

  template <typename T>
  opt<T> config_table::try_get_value(const std::string_view toml_path) const {
    PROFILE_SECTION("config_table::try_get_value");
    if (!has_path(toml_path)) {
      return std::nullopt;
    }

    if constexpr (std::is_same_v<T, toml::table>) {
      const toml::table* subtable = get_subtable(toml_path);
      if (subtable != nullptr) {
        return *subtable;
      } else {
        CORE_LOG_WARN("Config key '{}' is not a table.", toml_path);
      }
    } else {
      toml::node_view node = table.get().at_path(toml_path);
      OTHER_ASSERT(node, "Config key '{}' not found.", toml_path);

      if (check_type<T>(node, toml_path)) {
        return return_node<T>(node, toml_path);
      } else {
        CORE_LOG_WARN("Config key '{}' is not of the expected type.", toml_path);
      }
    }

    return std::nullopt;
  }

  /// the supported config value types; get_value<T> with a new T links against this list
  template opt<bool> config_table::try_get_value<bool>(const std::string_view) const;
  template opt<int> config_table::try_get_value<int>(const std::string_view) const;
  template opt<uint16_t> config_table::try_get_value<uint16_t>(const std::string_view) const;
  template opt<uint32_t> config_table::try_get_value<uint32_t>(const std::string_view) const;
  template opt<size_t> config_table::try_get_value<size_t>(const std::string_view) const;
  template opt<float> config_table::try_get_value<float>(const std::string_view) const;
  template opt<double> config_table::try_get_value<double>(const std::string_view) const;
  template opt<std::string> config_table::try_get_value<std::string>(const std::string_view) const;
  template opt<ostd::vector<std::string>> config_table::try_get_value<ostd::vector<std::string>>(const std::string_view) const;
  template opt<std::vector<int>> config_table::try_get_value<std::vector<int>>(const std::string_view) const;
  template opt<toml::table> config_table::try_get_value<toml::table>(const std::string_view) const;

  bool config_table::has_path(const std::string_view toml_path) const {
    return (bool)table.get().at_path(toml_path);
  }

  toml::node_view<const toml::node> config_table::get_raw(const std::string_view toml_path) const {
    return table.get().at_path(toml_path);
  }

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
    config.table.get() = toml::parse(contents);

    toml::node_view driver_path = config.table.get().at_path("application.driver");
    driver = driver_path.as_string() == nullptr ? "" : driver_path.as_string()->get();

    toml::node_view dotnet_modules = config.table.get().at_path("scripting.dotnet-modules");
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

    toml::node_view log_level_node = config.table.get().at_path("application.core-log-level");
    toml::node_view file_log_level_node = config.table.get().at_path("application.file-log-level");
    toml::node_view log_file_node = config.table.get().at_path("application.core-log-file");

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

    toml::node_view environment = config.table.get().at_path("environment");
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

    toml::node_view force_no_window = config.table.get().at_path("rendering.force-no-window");
    if (force_no_window.is_boolean()) {
      config.force_no_window = force_no_window.as_boolean();
    }

    toml::node_view rendering_backend = config.table.get().at_path("rendering.backend");
    rendering = rendering_backend.as_string() == nullptr ? "" : rendering_backend.as_string()->get();

    toml::node_view window_size_node = config.table.get().at_path("rendering.window-size");
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

    toml::node_view project_file = config.table.get().at_path("application.project-file");
    if (project_file.is_string()) {
      filepath path = filepath(project_file.as_string()->get());
      if (path.empty()) {
        std::println(std::cerr, "Invalid value for 'application.project-file', empty path provided.");
      } else if (!std::filesystem::exists(path)) {
        std::println(std::cerr, "Invalid value for 'application.project-file', file does not exist: {}", path.string());
      } else {
        config.project_file = path;
        /// config parsing runs before the logger subsystem boots; CORE_LOG_* here aborts
        std::println(std::cout, "Using project file: {}", path.string());
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
    toml::node_view node = table.get().at_path(std::format("project.{}", toml_path));
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
    return table.get();
  }

  const toml::table& config_table::get_project_table() const {
    return table.get();
  }

  std::string config_table::dump_table_string() const {
    PROFILE_SECTION("config_table::dump_table_string");
    std::ostringstream oss;
    oss << table.get();
    return oss.str();
  }

  const toml::table* config_table::get_subtable(const std::string_view toml_path) const {
    toml::node_view node = table.get().at_path(toml_path);
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
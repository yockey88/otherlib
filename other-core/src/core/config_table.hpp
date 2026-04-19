/**
 * \file core/config_table.hpp
 **/
#ifndef OTHER_CORE_CONFIG_TABLE_HPP
#define OTHER_CORE_CONFIG_TABLE_HPP

#include <string>
#include <string_view>

#include <glm/glm.hpp>
#include <toml++/toml.h>

#include "core/defines.hpp"
#include "core/profiler.hpp"
#include "core/value.hpp"
#include "serialization/reflection.hpp"

#include "defines.hpp"

namespace other {

class config_table {
public:
  static config_table load(const std::string_view filename);
  static config_table load_from_source(const std::string_view text);

  config_table() = default;
  config_table(const config_table &other) = default;
  config_table &operator=(const config_table &other) = default;

  ~config_table() = default;

  /**
   * \note this will look for the path @p toml_subpath at:
   *   [project.<toml_subpath>]
   */
  value get_project_value(const std::string_view toml_subpath) const;

  template <typename T>
  opt<T> try_get_value(const std::string_view toml_path) const {
    if constexpr (is_container<T> && !is_stringlike_type<T>) {
      return std::nullopt;
    } else {
      toml::node_view node = table.at_path(toml_path);
      if (!node) {
        return std::nullopt;
      }

      if (node.template is<T>()) {
        return node.template as<T>()->get();
      }
      return std::nullopt;
    }
  }

  toml::table &get_project_table();
  const toml::table &get_project_table() const;

  std::string dump_table_string() const;
  template <typename T>
  std::remove_cvref_t<T> get_value(const std::string_view toml_path,
                                   T default_value = {}) const {
    PROFILE_SECTION("config_table::get-value");
    opt<T> value_opt = try_get_value<T>(toml_path);
    if (!value_opt.has_value() &&
        !(is_container<T> && !is_stringlike_type<T>)) {
      if constexpr (is_string_type<T>) {
        return perform_tag_replacement(default_value);
      } else {
        return default_value;
      }
    }

    toml::node_view node = table.at_path(toml_path);
    if constexpr (is_container<T> && !is_stringlike_type<T>) {
      if (!node) {
        CORE_LOG_WARN("Config key '{}' not found, returning default value.",
                      toml_path);
        return default_value;
      } else {
        CORE_LOG_TRACE("Found config key '{}'", toml_path);
      }

      using value_type = typename T::value_type;

      T result;
      const toml::array *array_node = node.as_array();
      if (array_node == nullptr) {
        CORE_LOG_WARN(
            "Config key '{}' is not an array, returning default value.",
            toml_path);
        return result;
      }

      CORE_LOG_TRACE("Parsing config array for key '{}' ({} items)", toml_path,
                     array_node->size());
      array_node->for_each([&](auto &&elem) {
        if (!elem.template is<value_type>()) {
          CORE_LOG_WARN("Element in config array '{}' is not of the expected "
                        "type, skipping.",
                        toml_path);
          return;
        }
        CORE_LOG_TRACE(" - Parsed element in config array '{}'", toml_path);
        value_type v = elem.template as<value_type>()->get();
        if constexpr (is_string_type<value_type>) {
          v = perform_tag_replacement(v);
        }
        result.push_back(v);
      });

      return result;
    } else {
      CORE_LOG_TRACE("Parsing config value for key '{}'", toml_path);
      if (node.template is<T>()) {
        if constexpr (std::is_same_v<T, std::string>) {
          /// here we replace ${x} with environment variables/necessary
          /// replacements
          return perform_tag_replacement(node.template as<T>()->get());
        } else {
          return node.template as<T>()->get();
        }
      } else {
        CORE_LOG_WARN("Config key '{}' is not of the expected type, returning "
                      "default value.",
                      toml_path);
        if constexpr (std::is_same_v<T, std::string>) {
          return perform_tag_replacement(default_value);
        } else {
          return default_value;
        }
      }
    }

    if constexpr (is_stringlike_type<T>) {
      return perform_tag_replacement(default_value);
    } else {
      return default_value;
    }
  }

  inline const auto get_raw(const std::string_view toml_path) const {
    return table.at_path(toml_path);
  }
  const toml::table *get_subtable(const std::string_view toml_path) const;

  bool valid = true;
  struct {
    bool verbose = false;
  } diagnostics;

  //// application settings
  opt<std::string> dynamic_driver_rel_path;
  uint32_t core_log_level = (spdlog::level::level_enum)spdlog::level::warn;
  std::string core_log_file = "logs/other_env.log";

  /// environment settings
  bool open_terminal = false;

  /// rendering settings
  opt<std::string> rendering_backend;

  bool force_no_window = false;

  glm::uvec2 window_size = {1920, 1080};
  glm::vec4 clear_color = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);

private:
  friend opt<config_table> parse_raw_config(const std::string_view filename);
  toml::table table;
};

namespace configuration {
namespace section {

inline constexpr std::string_view kProject = "project";
inline constexpr std::string_view kApplication = "application";
inline constexpr std::string_view kRendering = "rendering";
inline constexpr std::string_view kScripting = "scripting";

} // namespace section
namespace key {

// application
inline constexpr std::string_view kDynamicDriverPath =
    "dynamic-driver-rel-path";

// rendering
inline constexpr std::string_view kRenderingBackend = "rendering-backend";
inline constexpr std::string_view kForceNoWindow = "force-no-window";
inline constexpr std::string_view kWindowSize = "window-size";
inline constexpr std::string_view kClearColor = "clear-color";

// scripting
inline constexpr std::string_view kDotnetBindings = "dotnet-bindings";
inline constexpr std::string_view kDotnetRuntimeConfig =
    "dotnet-runtime-config";
inline constexpr std::string_view kOtherCSharp = "other-csharp";

} // namespace key

constexpr std::string_view kProjectDynamicDriverPath =
    "project.dynamic-driver-rel-path";

constexpr std::string_view kRenderingBackend = "rendering.rendering-backend";
constexpr std::string_view kForceNoWindow = "rendering.force-no-window";
constexpr std::string_view kWindowSize = "rendering.window-size";
constexpr std::string_view kClearColor = "rendering.clear-color";

constexpr std::string_view kDotnetBindings = "scripting.dotnet-bindings";
constexpr std::string_view kDotnetRuntimeConfig =
    "scripting.dotnet-runtime-config";
constexpr std::string_view kOtherCSharp = "scripting.other-csharp";

} // namespace configuration
} // namespace other

#endif // OTHER_CORE_CONFIG_TABLE_HPP
/**
 * \file core/config_table.hpp
 **/
#ifndef OTHER_CORE_CONFIG_TABLE_HPP
#define OTHER_CORE_CONFIG_TABLE_HPP

#include <cstdint>
#include <string>
#include <string_view>

#include <glm/glm.hpp>
#include <toml++/toml.h>

#include "core/defines.hpp"
#include "core/profiler.hpp"
#include "core/value.hpp"
#include "serialization/reflection.hpp"

namespace other {

  class config_table;

  /// parse a toml string into a config table; nullopt on parse failure.
  /// (the friend declaration inside config_table alone is not found by ordinary lookup)
  opt<config_table> parse_string_config(const std::string_view contents);

  class config_table {
   public:
    static config_table load(const std::string_view filename);
    static config_table load_from_source(const std::string_view text);

    config_table() = default;
    config_table(const config_table& other) = default;
    config_table& operator=(const config_table& other) = default;

    ~config_table() = default;

    /**
     * \note this will look for the path @p toml_subpath at:
     *   [project.<toml_subpath>]
     */
    value get_project_value(const std::string_view toml_subpath) const;

    template <typename T>
    opt<T> try_get_value(const std::string_view toml_path) const {
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
        toml::node_view node = table.at_path(toml_path);
        OTHER_ASSERT(node, "Config key '{}' not found.", toml_path);

        if (check_type<T>(node, toml_path)) {
          return return_node<T>(node, toml_path);
        } else {
          CORE_LOG_WARN("Config key '{}' is not of the expected type.", toml_path);
        }
      }

      return std::nullopt;
    }

    toml::table& get_project_table();
    const toml::table& get_project_table() const;

    std::string dump_table_string() const;
    template <typename T>
    std::remove_cvref_t<T> get_value(const std::string_view toml_path, T default_value = {}) const {
      PROFILE_SECTION("config_table::get-value");
      using ret_t = std::remove_cvref_t<T>;
      if constexpr (is_stringlike_type<T>) {
        return try_get_value<ret_t>(toml_path).value_or(perform_tag_replacement(default_value));
      } else {
        return try_get_value<ret_t>(toml_path).value_or(std::move(default_value));
      }
    }

    inline bool has_path(const std::string_view toml_path) const {
      return (bool)table.at_path(toml_path);
    }

    inline const auto get_raw(const std::string_view toml_path) const {
      return table.at_path(toml_path);
    }
    const toml::table* get_subtable(const std::string_view toml_path) const;

    bool valid = true;
    struct {
      bool verbose = false;
    } diagnostics;

    //// application settings
    opt<std::string> dynamic_driver_rel_path;
    uint32_t core_log_level = (spdlog::level::level_enum)spdlog::level::warn;
    uint32_t file_log_level = (spdlog::level::level_enum)spdlog::level::trace;
    std::string core_log_file = "logs/other_env.log";

    /// environment settings
    bool open_terminal = false;

    /// rendering settings
    opt<std::string> rendering_backend;

    bool force_no_window = false;

    glm::uvec2 window_size = { 1920, 1080 };
    glm::vec4 clear_color = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);

    /// project settings
    opt<filepath> project_file = std::nullopt;

   private:
    template <typename T>
    bool check_type(auto n, const std::string_view toml_path) const {
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
    T return_node(auto n, const std::string_view toml_path) const {
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
        if constexpr (false) {
        }

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

    friend opt<config_table> parse_string_config(const std::string_view contents);
    toml::table table;
  };

  namespace configuration {
    namespace section {

      inline constexpr std::string_view kProject = "project";
      inline constexpr std::string_view kApplication = "application";
      inline constexpr std::string_view kRendering = "rendering";
      inline constexpr std::string_view kScripting = "scripting";

    }  // namespace section
    namespace key {

      // application
      inline constexpr std::string_view kDynamicDriverPath = "dynamic-driver-rel-path";

      // rendering
      inline constexpr std::string_view kRenderingBackend = "rendering-backend";
      inline constexpr std::string_view kForceNoWindow = "force-no-window";
      inline constexpr std::string_view kWindowSize = "window-size";
      inline constexpr std::string_view kClearColor = "clear-color";

      // scripting
      inline constexpr std::string_view kDotnetBindings = "dotnet-bindings";
      inline constexpr std::string_view kDotnetRuntimeConfig = "dotnet-runtime-config";
      inline constexpr std::string_view kOtherCSharp = "other-csharp";

    }  // namespace key

    constexpr std::string_view kProjectDynamicDriverPath = "project.dynamic-driver-rel-path";

    constexpr std::string_view kRenderingBackend = "rendering.rendering-backend";
    constexpr std::string_view kForceNoWindow = "rendering.force-no-window";
    constexpr std::string_view kWindowSize = "rendering.window-size";
    constexpr std::string_view kClearColor = "rendering.clear-color";

    constexpr std::string_view kDotnetBindings = "scripting.dotnet-bindings";
    constexpr std::string_view kDotnetRuntimeConfig = "scripting.dotnet-runtime-config";
    constexpr std::string_view kOtherCSharp = "scripting.other-csharp";

  }  // namespace configuration
}  // namespace other

#endif  // OTHER_CORE_CONFIG_TABLE_HPP
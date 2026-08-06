/**
 * \file core/config_table.hpp
 **/
#ifndef OTHER_CORE_CONFIG_TABLE_HPP
#define OTHER_CORE_CONFIG_TABLE_HPP

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>

#include <glm/glm.hpp>
#include <toml++/impl/forward_declarations.hpp>

#include "core/defines.hpp"
#include "core/profiler.hpp"
#include "core/value.hpp"
#include "serialization/reflection.hpp"

namespace other {

  class config_table;

  /// parse a toml string into a config table; nullopt on parse failure.
  /// (the friend declaration inside config_table alone is not found by ordinary lookup)
  opt<config_table> parse_string_config(const std::string_view contents);

  namespace detail {

    /// value-semantic heap box for the parsed table: special members are defined in
    ///  config_table.cpp so this header only needs toml++'s forward declarations,
    ///  while config_table itself keeps its defaulted memberwise copy/move
    class toml_table_box {
     public:
      toml_table_box();
      toml_table_box(const toml_table_box& other);
      toml_table_box& operator=(const toml_table_box& other);
      toml_table_box(toml_table_box&& other) noexcept;
      toml_table_box& operator=(toml_table_box&& other) noexcept;
      ~toml_table_box();

      toml::table& get() { return *ptr; }
      const toml::table& get() const { return *ptr; }

     private:
      std::unique_ptr<toml::table> ptr;
    };

  }  // namespace detail

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

    /// defined in config_table.cpp with explicit instantiations for the supported
    ///  value types; an unresolved external here means a new type needs a row there
    template <typename T>
    opt<T> try_get_value(const std::string_view toml_path) const;

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

    bool has_path(const std::string_view toml_path) const;

    /// callers dereferencing the view include <toml++/toml.h> themselves
    toml::node_view<const toml::node> get_raw(const std::string_view toml_path) const;
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
    friend opt<config_table> parse_string_config(const std::string_view contents);
    detail::toml_table_box table;
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
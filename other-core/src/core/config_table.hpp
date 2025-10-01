/**
 * \file core/config_table.hpp
 **/
#ifndef OTHER_CORE_CONFIG_TABLE_HPP
#define OTHER_CORE_CONFIG_TABLE_HPP

#include <string>
#include <string_view>

#include <glm/glm.hpp>

#include "core/defines.hpp"
#include "core/value.hpp"
#include "serialization/reflection.hpp"

#include "spdlog/common.h"

#include <toml++/toml.h>

namespace other {

  class config_table {
   public:
    static config_table load(const std::string_view filename);
    static config_table load_from_source(const std::string_view text);

    config_table() = default;
    config_table(const config_table& other);
    config_table& operator=(const config_table& other);

    ~config_table() = default;

    /// for retrieving user-defined settings in 'project' section
    value get_project_value(const std::string_view section, const std::string_view key) const;

    /// for debugging only, returns the entire project table
    toml::table& get_project_table();
    const toml::table& get_project_table() const;

    std::string format_table_string(const std::string_view section, const std::string_view key) const;

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

    /// terminal settings

    /// rendering settings
    opt<std::string> rendering_backend;
    glm::uvec2 window_size = { 1280, 720 };
    glm::vec4 clear_color = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);

   private:
    friend opt<config_table> parse_raw_config(const std::string_view filename);
    toml::table table;
  };

}  // namespace other

#endif  // OTHER_CORE_CONFIG_TABLE_HPP
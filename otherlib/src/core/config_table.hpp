/**
 * \file core/config_table.hpp
 **/
#ifndef OTHER_CORE_CONFIG_TABLE_HPP
#define OTHER_CORE_CONFIG_TABLE_HPP

#include <string>
#include <string_view>

#include "core/defines.hpp"
#include "core/value.hpp"

#include <toml++/toml.h>

namespace other {

  class OTHER_CLASS config_table {
   public:
    static config_table load(const std::string_view filename);
    ~config_table() = default;

    value get_value(const std::string_view section, const std::string_view key) const;

    bool valid = false;
    opt<std::string> dynamic_driver_rel_path;

   private:
    friend opt<config_table> parse_raw_config(const std::string_view filename);
    toml::table table;
  };

}  // namespace other

#endif  // OTHER_CORE_CONFIG_TABLE_HPP
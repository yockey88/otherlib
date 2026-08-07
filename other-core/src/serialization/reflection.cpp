/**
 * \file serialization/reflection.cpp
 **/
#include "serialization/reflection.hpp"

#include "core/profiler.hpp"

namespace other {

  std::string reflection_data::member::get_name() const {
    if (display_name.has_value()) {
      return *display_name;
    }
    return name;
  }

  bool type_database::has_type(const std::string_view type_name) const {
    PROFILE_SECTION("type_database::has_type");
    return std::ranges::find_if(data_map, [&](const auto& pair) {
             CORE_LOG_TRACE("Checking type '{}' against stored type '{}'.", strip_namespace(type_name), strip_namespace(pair.second.type_name));
             return strip_namespace(pair.second.type_name) == strip_namespace(type_name) &&
               ((get_namespace_string(pair.second.type_name) == get_namespace_string(type_name)) || get_namespace_string(type_name).empty());
           }) != data_map.end();
  }

  const reflection_data* type_database::get_reflection_data(const std::string_view type_name) {
    PROFILE_SECTION("type_database::get_reflection_data");
    auto itr = std::ranges::find_if(data_map, [&](const auto& pair) {
      CORE_LOG_TRACE("Checking type '{}' against stored type '{}'.", strip_namespace(type_name), strip_namespace(pair.second.type_name));
      return strip_namespace(pair.second.type_name) == strip_namespace(type_name) &&
        ((get_namespace_string(pair.second.type_name) == get_namespace_string(type_name)) || get_namespace_string(type_name).empty());
    });
    if (itr != data_map.end()) {
      return &itr->second;
    }
    return nullptr;
  }

  std::string type_database::get_namespace_string(const std::string_view full_name) const {
    size_t pos = full_name.find_last_of("::");
    if (pos != std::string_view::npos) {
      return std::string{ full_name.substr(0, pos) };
    }
    return {};
  }

  std::string type_database::strip_namespace(const std::string_view full_name) const {
    size_t pos = full_name.find_last_of("::");
    if (pos != std::string_view::npos) {
      return std::string{ full_name.substr(pos + 1) };
    }
    return std::string{ full_name };
  }

};  // namespace other
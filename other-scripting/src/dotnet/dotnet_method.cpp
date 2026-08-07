/**
 * \file dotnet/dotnet_method.cpp
 **/
#include "dotnet/dotnet_method.hpp"

#include "core/logger.hpp"
#include "core/profiler.hpp"

#include "dotnet/dotnet_host.hpp"
#include "dotnet/types.hpp"

namespace other {

  dotnet_type* dotnet_method::get_return_type() {
    OTHER_ASSERT(host != nullptr, "dotnet_host is null");
    if (return_type == nullptr) {
      int32_t return_type_id = -1;
      host->interop().get_method_return_type(dotnet_id, &return_type_id);
      return_type = host->get_type_cache()->cache_type(host, return_type_id);
    }
    return return_type;
  }

  void dotnet_method::initialize_method() {
    OTHER_ASSERT(host != nullptr, "dotnet_host is null");
    PROFILE_SECTION("dotnet_method::initialize_method");
    ostd::vector<int32_t> attribute_ids;
    int32_t num_attributes = 0;

    host->interop().get_method_attributes(dotnet_id, nullptr, &num_attributes);
    attribute_ids.resize(num_attributes);
    host->interop().get_method_attributes(dotnet_id, attribute_ids.data(), &num_attributes);

    CORE_LOG_TRACE("Initializing method [{}] with {} attributes", name(), attribute_ids.size());
    attributes.reserve(attribute_ids.size());
    for (int32_t attribute_id : attribute_ids) {
      int32_t attribute_type_id = -1;
      host->interop().get_attribute_type(attribute_id, &attribute_type_id);
      attributes.emplace_back(host, attribute_type_id, attribute_id);
    }
  }

  bool dotnet_method::is_static() const {
    OTHER_ASSERT(host != nullptr, "dotnet_host is null");
    nbool32 res = host->interop().is_method_static(dotnet_id);
    return res != 0;
  }

  bool dotnet_method::has_attribute(const std::string_view attr_name) const {
    return dotnet_attribute_has_dotnet_attribute(attributes, attr_name);
  }

  void dotnet_method::get_attribute(const std::string_view attr_name, const std::string_view field_name, void* out) const {
    if (!dotnet_attribute_get_attribute_object(host, attributes, attr_name, field_name, out)) {
      CORE_LOG_ERROR("Failed to get attribute object for attribute '{}' and field '{}'", attr_name, field_name);
    }
  }

  ostd::vector<std::string> dotnet_method::get_attribute_names() const {
    ostd::vector<std::string> names;
    for (const auto& attr : attributes) {
      names.push_back(attr.name());
    }
    return names;
  }

  std::string dotnet_method::name() const {
    native_string name_str = host->interop().get_method_name(dotnet_id);
    std::string res = name_str;
    native_string::free_str(name_str);
    return res;
  }

}  // namespace other
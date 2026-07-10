/**
 * \file dotnet/dotnet_attribute.cpp
 **/
#include "dotnet/dotnet_attribute.hpp"

#include "core/logger.hpp"

#include "dotnet/dotnet_host.hpp"
#include "dotnet/native_string.hpp"

namespace other {

  std::string dotnet_attribute::name() const {
    native_string name_str;
    if (type_dotnet_id != -1) {
      name_str = host->interop().get_full_type_name(type_dotnet_id);

    } else {
      CORE_LOG_ERROR("Attribute type ID is invalid: {}", type_dotnet_id);
      return {};
    }
    std::string res = name_str;
    native_string::free_str(name_str);
    return res;
  }

  bool dotnet_attribute_has_dotnet_attribute(const std::span<const dotnet_attribute> attributes, const std::string_view attr_name) {
    return find_dotnet_attribute(attributes, attr_name) != attributes.end();
  }

  std::span<const dotnet_attribute>::const_iterator find_dotnet_attribute(const std::span<const dotnet_attribute> attributes, const std::string_view attr_name) {
    if (auto itr = std::ranges::find_if(attributes, [&attr_name](const dotnet_attribute& attr) { return attr.name() == attr_name; }); itr != attributes.end()) {
      return itr;
    }

    if (!attr_name.ends_with("Attribute")) {
      // Check for the attribute without the "Attribute" suffix
      std::string attr_name_w_suffix = std::string{ attr_name } + "Attribute";
      if (auto itr = std::ranges::find_if(attributes, [&attr_name_w_suffix](const dotnet_attribute& attr) { return attr.name() == attr_name_w_suffix; }); itr != attributes.end()) {
        CORE_LOG_TRACE("Found attribute with 'Attribute' suffix: {}", attr_name_w_suffix);
        return itr;
      }
    }

    if (!attr_name.starts_with("System.") && !attr_name.starts_with("Other.")) {
      // Check for the attribute with "System." or "Other." prefix
      std::string attr_name_sys = "System." + std::string{ attr_name };
      std::string attr_name_other = "Other." + std::string{ attr_name };
      if (auto itr = std::ranges::find_if(attributes, [&attr_name_sys](const dotnet_attribute& attr) { return attr.name() == attr_name_sys; }); itr != attributes.end()) {
        CORE_LOG_TRACE("Found attribute with 'System.' prefix: {}", attr_name_sys);
        return itr;
      }
      if (auto itr = std::ranges::find_if(attributes, [&attr_name_other](const dotnet_attribute& attr) { return attr.name() == attr_name_other; }); itr != attributes.end()) {
        CORE_LOG_TRACE("Found attribute with 'Other.' prefix: {}", attr_name_other);
        return itr;
      }
    }

    /// now do System.Attribute and Other.Attribute checks
    std::string attr_name_sys = "System." + std::string{ attr_name } + "Attribute";
    std::string attr_name_other = "Other." + std::string{ attr_name } + "Attribute";
    if (auto itr = std::ranges::find_if(attributes, [&attr_name_sys](const dotnet_attribute& attr) { return attr.name() == attr_name_sys; }); itr != attributes.end()) {
      CORE_LOG_TRACE("Found attribute with 'System.' prefix and 'Attribute' suffix: {}", attr_name_sys);
      return itr;
    }
    if (auto itr = std::ranges::find_if(attributes, [&attr_name_other](const dotnet_attribute& attr) { return attr.name() == attr_name_other; }); itr != attributes.end()) {
      CORE_LOG_TRACE("Found attribute with 'Other.' prefix and 'Attribute' suffix: {}", attr_name_other);
      return itr;
    }

    return attributes.end();
  }

  bool dotnet_attribute_get_attribute_object(dotnet_host* host, const std::span<const dotnet_attribute> attributes, const std::string_view attr_name, const std::string_view field_name, void* out) {
    OTHER_ASSERT(host != nullptr, "dotnet_host is null");

    auto itr = find_dotnet_attribute(attributes, attr_name);
    if (itr == attributes.end()) {
      CORE_LOG_ERROR("Attribute '{}' not found on object", attr_name);
      return false;
    }

    native_string field = native_string::new_str(field_name);
    host->interop().get_attribute_object(itr->dotnet_id, field, out);
    native_string::free_str(field);
    return true;
  }

}  // namespace other
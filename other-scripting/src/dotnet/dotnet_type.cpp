/**
 * \file dotnet/dotnet_type.cpp
 **/
#include "dotnet/dotnet_type.hpp"

#include <ranges>
#include <string>

#include "dotnet/dotnet_assembly.hpp"
#include "dotnet/dotnet_object.hpp"
#include "dotnet/host.hpp"

namespace other {

  void dotnet_type::initialize_type_interface() {
    OTHER_ASSERT(host != nullptr, "dotnet_host is null");
    if (dotnet_id == -1 || type_interface_initialized) {
      return;
    }

    dotnet_methods.clear();
    dotnet_fields.clear();
    dotnet_attributes.clear();

    {
      std::vector<int32_t> dotnet_attribute_ids;
      fill_out_type_information(dotnet_attribute_ids, host->interop().get_attributes);

      dotnet_attributes.reserve(dotnet_attribute_ids.size());
      for (int32_t attribute_id : dotnet_attribute_ids) {
        int32_t attribute_type_id = -1;
        host->interop().get_attribute_type(attribute_id, &attribute_type_id);
        dotnet_attributes.emplace_back(attribute_data{ attribute_type_id, { host, attribute_type_id, attribute_id } });
      }
    }
    {
      std::vector<int32_t> dotnet_method_ids;
      fill_out_type_information(dotnet_method_ids, host->interop().get_type_methods);

      dotnet_methods.reserve(dotnet_method_ids.size());
      for (int32_t method_id : dotnet_method_ids) {
        dotnet_methods.emplace_back(host, this, method_id);
      }
    }
    {
      std::vector<int32_t> dotnet_field_ids;
      fill_out_type_information(dotnet_field_ids, host->interop().get_type_fields);
      std::vector<int32_t> dotnet_property_ids;
      fill_out_type_information(dotnet_property_ids, host->interop().get_type_properties);

      dotnet_fields.reserve(dotnet_field_ids.size() + dotnet_property_ids.size());
      for (int32_t field_id : dotnet_field_ids) {
        auto& f = dotnet_fields.emplace_back(host, this, field_id);
        f.initialize_field();
      }
      for (int32_t property_id : dotnet_property_ids) {
        auto& p = dotnet_fields.emplace_back(host, this, property_id, true);
        p.initialize_field();
      }
    }

    type_interface_initialized = true;

    CORE_LOG_DEBUG("dotnet_type[{}] initialized", full_name());
  }

  std::string dotnet_type::full_name() const {
    native_string name = host->interop().get_full_type_name(dotnet_id);
    std::string res = name;
    native_string::free_str(name);
    return res;
  }

  std::string dotnet_type::namespace_name() const {
    std::string full_name_str = full_name();
    size_t last_dot = full_name_str.rfind('.');
    std::string res = (last_dot == std::string::npos) ? "" : full_name_str.substr(0, last_dot);
    return res;
  }

  std::string dotnet_type::class_name() const {
    std::string gull_name = full_name();
    size_t last_dot = gull_name.rfind('.');
    std::string res = (last_dot == std::string::npos) ? gull_name : gull_name.substr(last_dot + 1);
    return res;
  }

  bool dotnet_type::has_attribute(const std::string_view attr_name) const {
    OTHER_ASSERT(host != nullptr, "dotnet_host is null");
    if (std::ranges::find_if(dotnet_attributes, [&attr_name](const attribute_data& attr) { return attr.attribute.name() == attr_name; }) != dotnet_attributes.end()) {
      return true;
    }

    if (!attr_name.ends_with("Attribute")) {
      // Check for the attribute without the "Attribute" suffix
      std::string attr_name_w_suffix = std::string{ attr_name } + "Attribute";
      if (std::ranges::find_if(dotnet_attributes, [&attr_name_w_suffix](const attribute_data& attr) { return attr.attribute.name() == attr_name_w_suffix; }) != dotnet_attributes.end()) {
        return true;
      }
    }

    if (!attr_name.starts_with("System.") && !attr_name.starts_with("Other.")) {
      // Check for the attribute with "System." or "Other." prefix
      std::string attr_name_sys = "System." + std::string{ attr_name };
      std::string attr_name_other = "Other." + std::string{ attr_name };
      if (std::ranges::find_if(dotnet_attributes, [&attr_name_sys](const attribute_data& attr) { return attr.attribute.name() == attr_name_sys; }) != dotnet_attributes.end() ||
          std::ranges::find_if(dotnet_attributes, [&attr_name_other](const attribute_data& attr) { return attr.attribute.name() == attr_name_other; }) != dotnet_attributes.end()) {
        return true;
      }
    }

    /// now do System.Attribute and Other.Attribute checks
    std::string attr_name_sys = "System." + std::string{ attr_name } + "Attribute";
    std::string attr_name_other = "Other." + std::string{ attr_name } + "Attribute";
    if (std::ranges::find_if(dotnet_attributes, [&attr_name_sys](const attribute_data& attr) { return attr.attribute.name() == attr_name_sys; }) != dotnet_attributes.end() ||
        std::ranges::find_if(dotnet_attributes, [&attr_name_other](const attribute_data& attr) { return attr.attribute.name() == attr_name_other; }) != dotnet_attributes.end()) {
      return true;
    }

    return false;
  }

  std::vector<std::string> dotnet_type::get_attribute_names() const {
    std::vector<std::string> names;
    for (const auto& attr : dotnet_attributes) {
      names.push_back(attr.attribute.name());
    }
    return names;
  }

  bool dotnet_type::has_field(const std::string_view field_name) const {
    OTHER_ASSERT(host != nullptr, "dotnet_host is null");
    OTHER_ASSERT(dotnet_id != -1, "dotnet_id is invalid: {}", dotnet_id);

    if (field_name.empty()) {
      CORE_LOG_ERROR("Field name cannot be empty");
      return false;
    }

    native_string field_name_str = native_string::new_str(field_name);
    bool has_field = host->interop().has_field(dotnet_id, field_name_str);
    // bool has_field = false;
    native_string::free_str(field_name_str);
    return has_field;
  }

  bool dotnet_type::is_field_property(const std::string_view field_name) const {
    OTHER_ASSERT(host != nullptr, "dotnet_host is null");
    OTHER_ASSERT(dotnet_id != -1, "dotnet_id is invalid: {}", dotnet_id);

    if (field_name.empty()) {
      CORE_LOG_ERROR("Field name cannot be empty");
      return false;
    }

    auto itr = std::ranges::find_if(dotnet_fields, [&field_name](const dotnet_field& field) {
      return field.name() == field_name;
    });
    if (itr == dotnet_fields.end()) {
      CORE_LOG_ERROR("Field '{}' not found in type '{}'", field_name, full_name());
      return false;
    }
    return itr->is_property();
  }

  dotnet_object* dotnet_type::instantiate_object(const std::string_view name, const void** argv, const managed_type* arg_ts, size_t argc) {
    OTHER_ASSERT(host != nullptr, "dotnet_host is null");
    if (dotnet_id == -1) {
      CORE_LOG_ERROR("Cannot instantiate object of type with invalid dotnet_id: {}", dotnet_id);
      return nullptr;
    }

    return host->instantiate_managed_object_of_type(name, this, argv, arg_ts, argc);
  }

  void dotnet_type::destroy_object(dotnet_object* obj) {
    OTHER_ASSERT(host != nullptr, "dotnet_host is null");
    OTHER_ASSERT(obj != nullptr, "dotnet_object is null");
    OTHER_ASSERT(obj->managed_object != nullptr, "dotnet_object has no managed object associated with it");

    host->destroy_managed_object(obj);
  }

  sol::table dotnet_type::create_lua_descriptor(sol::state& lua_state) {
    sol::table type_table = lua_state.create_table_with(
      "FullTypeName", full_name(),
      /// this includes all namespaces except the class name, e.g. "System.Collections.Generic"
      "NameSpace", namespace_name(),
      "ClassName", class_name(),
      "DotNetID", dotnet_id
    );

    type_table["Attributes"] = lua_state.create_table();
    type_table["Methods"] = lua_state.create_table();
    type_table["Fields"] = lua_state.create_table();

    for (const auto& attr : dotnet_attributes) {
      type_table["Attributes"][attr.attribute.name()] = lua_state.create_table_with(
        "Name", attr.attribute.name(),
        "DotNetID", attr.attribute.dotnet_id
      );
    }

    for (auto& method : dotnet_methods) {
      sol::table method_table = type_table["Methods"][method.name()] = lua_state.create_table_with(
        "Name", method.name(),
        "DotNetID", method.dotnet_id
      );
      /// \todo
      // method_table["ReturnType"] = method.get_return_type();

      /// \todo
      // sol::table param_table = method_table["Parameters"] = lua_state.create_table();
      // const auto& params = method.get_parameters();
      // for (size_t i = 0; i < params.size(); ++i) {
      //   const auto& param = params[i];
      //   sol::table ptable = param_table[i + 1] = lua_state.create_table_with(
      //     "Name", param.name(),
      //     "Type", param.get_type(),
      //     "IsOut", param.is_out(),
      //     "IsRef", param.is_ref()
      //   );
      // }
    }

    for (const auto& field : dotnet_fields) {
      sol::table field_table = type_table["Fields"][field.name()] = lua_state.create_table_with(
        "Name", field.name(),
        "Type", field.get_type(),
        "DotNetID", field.dotnet_id,
        "IsProperty", field.is_property()
      );

      value val = field.get_default_value();
      switch (val.type()) {
        case value_type::CHAR: field_table["Value"] = (char)val; break;
        case value_type::OEBOOL: field_table["Value"] = (bool)val; break;
        case value_type::INT8: field_table["Value"] = (int8_t)val; break;
        case value_type::INT16: field_table["Value"] = (int16_t)val; break;
        case value_type::INT32: field_table["Value"] = (int32_t)val; break;
        case value_type::INT64: field_table["Value"] = (int64_t)val; break;
        case value_type::UINT8: field_table["Value"] = (uint8_t)val; break;
        case value_type::UINT16: field_table["Value"] = (uint16_t)val; break;
        case value_type::UINT32: field_table["Value"] = (uint32_t)val; break;
        case value_type::UINT64: field_table["Value"] = (uint64_t)val; break;
        case value_type::FLOAT: field_table["Value"] = (float)val; break;
        case value_type::DOUBLE: field_table["Value"] = (double)val; break;
        case value_type::STRING: field_table["Value"] = (std::string)val; break;
        default:
          // CORE_LOG_WARN("Unsupported default value type '{}' for field '{}'", static_cast<int>(val.type()), field.name());
          break;
      }
    }

    return type_table;
  }

  void dotnet_type::fill_out_type_information(std::vector<int32_t>& dotnet_ids, get_type_information fn) {
    OTHER_ASSERT(fn != nullptr, "get_type_information function is null");

    int32_t count = 0;
    fn(dotnet_id, nullptr, &count);

    dotnet_ids.resize(count);
    fn(dotnet_id, dotnet_ids.data(), &count);
    OTHER_ASSERT(dotnet_ids.size() == count, "Mismatch in expected type information count");
  }

  void dotnet_type::get_attribute_object(const std::string_view name, const std::string_view field_name, void* out) const {
    OTHER_ASSERT(host != nullptr, "dotnet_host is null");

    if (!has_attribute(name)) {
      CORE_LOG_ERROR("Attribute '{}' not found in type '{}'", name, full_name());
      return;
    }
    auto itr = std::ranges::find_if(dotnet_attributes, [&name](const attribute_data& attr) { return attr.attribute.name() == name; });
    if (itr == dotnet_attributes.end()) {
      CORE_LOG_ERROR("Attribute '{}' not found in type '{}'", name, full_name());
      return;
    }

    native_string field = native_string::new_str(field_name);
    host->interop().get_attribute_object(itr->attribute.dotnet_id, field, out);
    native_string::free_str(field);
  }

}  // namespace other

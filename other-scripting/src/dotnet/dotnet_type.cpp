/**
 * \file dotnet/dotnet_type.cpp
 **/
#include "dotnet/dotnet_type.hpp"

#include <ranges>
#include <string>

#include "core/profiler.hpp"

#include "dotnet/dotnet_assembly.hpp"
#include "dotnet/dotnet_host.hpp"
#include "dotnet/dotnet_object.hpp"


namespace other {

  void dotnet_type::initialize_type_interface() {
    OTHER_ASSERT(host != nullptr, "dotnet_host is null");
    PROFILE_SECTION("dotnet_type::initialize_type_interface");
    if (dotnet_id == -1 || type_interface_initialized) {
      return;
    }
    // CORE_LOG_DEBUG("Initializing dotnet_type[{}] interface", full_name());

    dotnet_methods.clear();
    dotnet_fields.clear();
    dotnet_attributes.clear();

    {
      PROFILE_SECTION("dotnet_type::initialize_type_interface--attributes");
      ostd::vector<int32_t> dotnet_attribute_ids;
      fill_out_type_information(dotnet_attribute_ids, host->interop().get_attributes);

      dotnet_attributes.reserve(dotnet_attribute_ids.size());
      for (int32_t attribute_id : dotnet_attribute_ids) {
        int32_t attribute_type_id = -1;
        host->interop().get_attribute_type(attribute_id, &attribute_type_id);
        dotnet_attributes.emplace_back(host, attribute_type_id, attribute_id);
      }
    }

    {
      PROFILE_SECTION("dotnet_type::initialize_type_interface--methods");
      ostd::vector<int32_t> dotnet_method_ids;
      fill_out_type_information(dotnet_method_ids, host->interop().get_type_methods);

      dotnet_methods.reserve(dotnet_method_ids.size());
      for (int32_t method_id : dotnet_method_ids) {
        dotnet_methods.emplace_back(host, this, method_id);
        dotnet_methods.back().initialize_method();
      }
    }
    {
      PROFILE_SECTION("dotnet_type::initialize_type_interface--fields");
      ostd::vector<int32_t> dotnet_field_ids;
      fill_out_type_information(dotnet_field_ids, host->interop().get_type_fields);

      ostd::vector<int32_t> dotnet_property_ids;
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

    // CORE_LOG_DEBUG("dotnet_type[{}] initialized", full_name());
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
    return dotnet_attribute_has_dotnet_attribute(dotnet_attributes, attr_name);
  }

  ostd::vector<std::string> dotnet_type::get_attribute_names() const {
    ostd::vector<std::string> names;
    for (const auto& attr : dotnet_attributes) {
      names.push_back(attr.name());
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

  bool dotnet_type::has_method(const std::string_view method_name) const {
    OTHER_ASSERT(host != nullptr, "dotnet_host is null");
    OTHER_ASSERT(dotnet_id != -1, "dotnet_id is invalid: {}", dotnet_id);

    if (method_name.empty()) {
      CORE_LOG_ERROR("Method name cannot be empty");
      return false;
    }

    native_string method_name_str = native_string::new_str(method_name);
    bool has_method = host->interop().has_method(dotnet_id, method_name_str);
    native_string::free_str(method_name_str);
    return has_method;
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

  void dotnet_type::fill_out_type_information(ostd::vector<int32_t>& dotnet_ids, get_type_information fn) {
    OTHER_ASSERT(fn != nullptr, "get_type_information function is null");

    int32_t count = 0;
    fn(dotnet_id, nullptr, &count);

    dotnet_ids.resize(count);
    fn(dotnet_id, dotnet_ids.data(), &count);
    OTHER_ASSERT(dotnet_ids.size() == count, "Mismatch in expected type information count");
  }

  void dotnet_type::get_attribute_object(const std::string_view name, const std::string_view field_name, void* out) const {
    if (!dotnet_attribute_get_attribute_object(host, dotnet_attributes, name, field_name, out)) {
      CORE_LOG_ERROR("Failed to get attribute object for attribute '{}' and field '{}'", name, field_name);
    }
  }

}  // namespace other

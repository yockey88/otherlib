/**
 * \file dotnet/dotnet_object.cpp
 **/
#include "dotnet/dotnet_object.hpp"

#include <string>

#include "core/fnv.hpp"

#include "dotnet/host.hpp"
#include "dotnet/native_string.hpp"

namespace other {

  bool dotnet_object::has_attribute(const std::string_view attr_name) {
    OTHER_ASSERT(host != nullptr, "dotnet_host is null");
    return dn_type->has_attribute(attr_name);
  }

  std::vector<std::string> dotnet_object::get_attribute_names() const {
    return dn_type->get_attribute_names();
  }

  void dotnet_object::write_storage_to_field(std::map<uint64_t, dotnet_field::storage>::const_iterator itr, const std::string_view field_name) {
    OTHER_ASSERT(host != nullptr, "dotnet_host is null");
    OTHER_ASSERT(managed_object != nullptr, "Object handle is null");

    if (itr->second.data == nullptr) {
      CORE_LOG_ERROR("Field '{}' data is null", field_name);
      return;
    }

    native_string name = native_string::new_str(field_name);
    bool is_property = dn_type->is_field_property(field_name);
    bool is_string = itr->second.stored_type == value_type::STRING;
    if (is_string) {
      std::string str(reinterpret_cast<const char*>(itr->second.data), itr->second.size - 1);
      native_string str_native;
      str_native = native_string::new_str(str);
      if (is_property) {
        host->interop().set_string_property(managed_object, name, &str_native);
      } else {
        host->interop().set_string_field(managed_object, name, &str_native);
      }
      native_string::free_str(str_native);
    } else {
      if (is_property) {
        host->interop().set_property(managed_object, name, itr->second.data);
      } else {
        host->interop().set_field(managed_object, name, itr->second.data);
      }
    }
    native_string::free_str(name);
  }

  size_t dotnet_object::managed_strlen(const std::string_view field_name) {
    OTHER_ASSERT(host != nullptr, "dotnet_host is null");
    OTHER_ASSERT(managed_object != nullptr, "Object handle is null");

    if (!type_has_field(field_name)) {
      CORE_LOG_ERROR("Field '{}' not found in type '{}'", field_name, dn_type->full_name());
      return 0;
    }

    native_string name = native_string::new_str(field_name);
    size_t len = 0;
    if (dn_type->is_field_property(field_name)) {
      len = host->interop().get_string_property_length(managed_object, name);
    } else {
      len = host->interop().get_string_field_length(managed_object, name);
    }
    native_string::free_str(name);
    return len;
  }

  bool dotnet_object::type_has_field(const std::string_view field_name) {
    OTHER_ASSERT(host != nullptr, "dotnet_host is null");
    if (dn_type == nullptr) {
      CORE_LOG_ERROR("Type is not initialized for dotnet_object '{}'", object_name);
      return false;
    }

    return dn_type->has_field(field_name);
  }

  void dotnet_object::load_field_into_storage(const std::string_view field_name, dotnet_field::storage& storage) {
    OTHER_ASSERT(host != nullptr, "dotnet_host is null");
    OTHER_ASSERT(managed_object != nullptr, "Object handle is null");

    native_string name = native_string::new_str(field_name);
    bool is_property = dn_type->is_field_property(field_name);
    bool is_string = storage.stored_type == value_type::STRING;

    if (is_string) {
      native_string str_native;
      if (is_property) {
        host->interop().get_string_property(managed_object, name, &str_native);
      } else {
        host->interop().get_string_field(managed_object, name, &str_native);
      }

      std::string str = str_native;
      native_string::free_str(str_native);
      storage.size = str.size() + 1;
      storage.data = (uint8_t*)arena::allocate(storage.size);
      std::memcpy(storage.data, str.data(), str.size());
      storage.data[storage.size - 1] = '\0';
    } else {
      if (is_property) {
        host->interop().get_property(managed_object, name, (void*)storage.data);
      } else {
        host->interop().get_field(managed_object, name, (void*)storage.data);
      }
    }
    native_string::free_str(name);
  }

  void dotnet_object::invoke_method_with_args(const std::string_view method_name, const void** argv, const managed_type* arg_ts, size_t argc) {
    OTHER_ASSERT(host != nullptr, "dotnet_host is null");
    auto name = native_string::new_str(method_name);
    host->interop().invoke_method(managed_object, name, argv, arg_ts, argc);
    native_string::free_str(name);
  }

  void dotnet_object::invoke_returning_method_args(const std::string_view method_name, const void** argv, const managed_type* arg_ts, size_t argc, void* out) {
    OTHER_ASSERT(host != nullptr, "dotnet_host is null");
    auto name = native_string::new_str(method_name);
    host->interop().invoke_method_ret(managed_object, name, argv, arg_ts, argc, out);
    native_string::free_str(name);
  }

}  // namespace other
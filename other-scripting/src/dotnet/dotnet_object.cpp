/**
 * \file dotnet/dotnet_object.cpp
 **/
#include "dotnet/dotnet_object.hpp"

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
    if (dn_type->is_field_property(field_name)) {
      OTHER_ASSERT(storage.data != nullptr, "Failed to allocate memory for field value");
      host->interop().get_property(managed_object, field_name, (void*)storage.data);
    } else {
      host->interop().get_field(managed_object, field_name, (void*)storage.data);
    }
    native_string::free_str(name);
  }

  void dotnet_object::invoke_method_with_args(const std::string_view method_name, const void** argv, const managed_type* arg_ts, size_t argc, opt<void*> ret) {
    OTHER_ASSERT(host != nullptr, "dotnet_host is null");
    auto name = native_string::new_str(method_name);
    if (ret) {
      host->interop().invoke_method_ret(managed_object, name, argv, arg_ts, argc, *ret);
    } else {
      host->interop().invoke_method(managed_object, name, argv, arg_ts, argc);
    }
    native_string::free_str(name);
  }

}  // namespace other
/**
 * \file dotnet/dotnet_type.cpp
 **/
#include "dotnet/dotnet_type.hpp"

#include "dotnet/host.hpp"

namespace other {

  std::string dotnet_type::full_name() {
    native_string name = host->interop().get_full_type_name(dotnet_id);
    std::string res = name;
    native_string::free_str(name);
    return res;
  }

  void dotnet_type::initialize_type_interface() {
    if (dotnet_id == -1 || type_interface_initialized) {
      return;
    }

    int32_t count = 0;
    /// methods
    {
      host->interop().get_type_methods(dotnet_id, nullptr, &count);
      method_dotnet_ids.resize(count);
      host->interop().get_type_methods(dotnet_id, method_dotnet_ids.data(), &count);
    }

    count = 0;
    /// fields
    {
      host->interop().get_type_fields(dotnet_id, nullptr, &count);
      field_dotnet_ids.resize(count);
      host->interop().get_type_methods(dotnet_id, field_dotnet_ids.data(), &count);
    }

    count = 0;
    /// properties
    {
      host->interop().get_type_properties(dotnet_id, nullptr, &count);
      property_dotnet_ids.resize(count);
      host->interop().get_type_methods(dotnet_id, property_dotnet_ids.data(), &count);
    }

    count = 0;
    /// attributes
    {
      host->interop().get_attributes(dotnet_id, nullptr, &count);
      attribute_dotnet_ids.resize(count);
      host->interop().get_type_methods(dotnet_id, attribute_dotnet_ids.data(), &count);
    }

    type_interface_initialized = true;

    CORE_LOG_DEBUG("dotnet_type[{}] initialized", full_name());
  }

}  // namespace other
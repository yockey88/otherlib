/**
 * \file dotnet/dotnet_attribute.cpp
 **/
#include "dotnet/dotnet_attribute.hpp"

#include "core/logger.hpp"

#include "dotnet/host.hpp"
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

}  // namespace other
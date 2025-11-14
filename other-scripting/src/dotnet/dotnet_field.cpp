/**
 * \file dotnet/dotnet_field.cpp
 **/
#include "dotnet/dotnet_field.hpp"

#include <cstring>

#include "core/arena.hpp"
#include "core/logger.hpp"

#include "dotnet/host.hpp"
#include "dotnet/native_string.hpp"

namespace other {

  void dotnet_field::storage::load_from_bytes(const uint8_t* new_data, uint64_t data_size) {
    if (data == nullptr || data_size > size) {
      arena::free(data, size);
      size = data_size;
      data = (uint8_t*)arena::allocate(size);
    }
    std::memset(data, 0, size);
    std::memcpy(data, new_data, data_size);
  }

  void dotnet_field::storage::load_from_value(const value& val) {
    load_from_bytes(reinterpret_cast<const uint8_t*>(val.read_storage().data()), val.size());
    stored_type = val.type();
  }

  void dotnet_field::storage::copy_string_to_storage(const std::string& value) {
    size_t new_size = value.size() + 1;
    if (new_size > size) {
      arena::free(data, size);
      size = new_size;
      data = (uint8_t*)arena::allocate(size);
    } else {
      std::memset(data, 0, size);
    }
    std::memcpy(data, value.data(), size);
    data[size - 1] = '\0';  // Ensure null termination
  }

  void dotnet_field::initialize_field() {
    OTHER_ASSERT(host != nullptr, "dotnet_host is null");
    host->interop().get_field_value_type(dotnet_id, (uint8_t*)&valtype);
  }

  std::string dotnet_field::name() const {
    OTHER_ASSERT(host != nullptr, "dotnet_host is null");
    native_string name_str;
    if (flags.is_property) {
      name_str = host->interop().get_property_name(dotnet_id);
    } else {
      name_str = host->interop().get_field_name(dotnet_id);
    }
    std::string res = name_str;
    native_string::free_str(name_str);
    return res;
  }

  value_type dotnet_field::get_type() const {
    return valtype;
  }

}  // namespace other
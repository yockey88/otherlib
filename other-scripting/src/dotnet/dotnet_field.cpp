/**
 * \file dotnet/dotnet_field.cpp
 **/
#include "dotnet/dotnet_field.hpp"

#include <cstring>

#include "core/logger.hpp"
#include "memory/arena.hpp"

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
    std::vector<int32_t> attribute_ids;
    int32_t num_attributes = 0;

    host->interop().get_field_attributes(dotnet_id, nullptr, &num_attributes);
    attribute_ids.resize(num_attributes);
    host->interop().get_field_attributes(dotnet_id, attribute_ids.data(), &num_attributes);

    attributes.reserve(attribute_ids.size());
    for (int32_t attribute_id : attribute_ids) {
      int32_t attribute_type_id = -1;
      host->interop().get_attribute_type(attribute_id, &attribute_type_id);
      attributes.emplace_back(host, attribute_type_id, attribute_id);
    }

    host->interop().get_field_value_type(dotnet_id, (uint8_t*)&valtype);
  }

  value dotnet_field::get_default_value() const {
    value val{};

    auto retrieve_default = [this]<typename T>() -> T {
      T v{};
      host->interop().get_default_value(dotnet_id, (void*)&v);
      return v;
    };

    switch (valtype) {
      case value_type::CHAR: val = retrieve_default.operator()<char>(); break;
      case value_type::OEBOOL: val = retrieve_default.operator()<bool>(); break;
      case value_type::INT8: val = retrieve_default.operator()<int8_t>(); break;
      case value_type::INT16: val = retrieve_default.operator()<int16_t>(); break;
      case value_type::INT32: val = retrieve_default.operator()<int32_t>(); break;
      case value_type::INT64: val = retrieve_default.operator()<int64_t>(); break;
      case value_type::UINT8: val = retrieve_default.operator()<uint8_t>(); break;
      case value_type::UINT16: val = retrieve_default.operator()<uint16_t>(); break;
      case value_type::UINT32: val = retrieve_default.operator()<uint32_t>(); break;
      case value_type::UINT64: val = retrieve_default.operator()<uint64_t>(); break;
      case value_type::FLOAT: val = retrieve_default.operator()<float>(); break;
      case value_type::DOUBLE: val = retrieve_default.operator()<double>(); break;
      case value_type::STRING: {
        native_string str_native;
        host->interop().get_default_value(dotnet_id, (void*)&str_native);
        std::string str = str_native;
        native_string::free_str(str_native);
        val = str;
      } break;
      default:
        // CORE_LOG_WARN("Default value retrieval not implemented for value_type {}", valtype);
        break;
    }

    return val;
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
/**
 * \file dotnet/dotnet_object.cpp
 **/
#include "dotnet/dotnet_object.hpp"

#include <print>
#include <string>

#include "core/fnv.hpp"
#include "serialization/serialization.hpp"

#include "dotnet/host.hpp"
#include "dotnet/native_string.hpp"

namespace other {

  std::string dotnet_object::get_type_name() const {
    OTHER_ASSERT(dn_type != nullptr, "Type is not initialized for dotnet_object '{}'", object_name);
    return dn_type->full_name();
  }

  bool dotnet_object::has_attribute(const std::string_view attr_name) {
    OTHER_ASSERT(host != nullptr, "dotnet_host is null");
    return dn_type->has_attribute(attr_name);
  }

  std::vector<std::string> dotnet_object::get_attribute_names() const {
    return dn_type->get_attribute_names();
  }

  std::vector<uint8_t> dotnet_object::serialize_to_bytes() {
    OTHER_ASSERT(host != nullptr, "dotnet_host is null");
    OTHER_ASSERT(managed_object != nullptr, "Object handle is null");

    std::vector<uint8_t> bytes = {};

    /**
    | num fields | fields |
    |---------------------|
    | 2 bytes    |        |
    **/
    /**
    | field name len | field name | field type | field data len | field data |
    |------------------------------------------------------------------------|
    | 2 byte         |            | 1 byte     | 8 bytes        |            |
    **/

    const auto& fields = dn_type->get_fields();

    serialization::write_value<uint16_t>((uint16_t)fields.size(), bytes);
    for (const auto& f : fields) {
      auto storage_itr = load_field(f.name(), f.get_type());
      OTHER_ASSERT(storage_itr != field_storage.end(), "Failed to load field storage for field '{}'", f.name());
      serialization::write_value<uint16_t>((uint16_t)f.name().size(), bytes);
      serialization::write_string_value(f.name(), bytes);
      serialization::write_value<uint8_t>((uint8_t)storage_itr->second.stored_type, bytes);
      serialization::write_value<uint64_t>(storage_itr->second.size, bytes);
      serialization::write_bytes(storage_itr->second.data, storage_itr->second.size, bytes);
    }

    return bytes;
  }

  void dotnet_object::load_from_bytes(const std::span<const uint8_t> buffer) {
    OTHER_ASSERT(host != nullptr, "dotnet_host is null!");
    OTHER_ASSERT(managed_object != nullptr, "Object handle is null!");

    uint64_t cursor = 0;

    uint16_t num_fields = serialization::read_value<uint16_t>(buffer, cursor);
    const auto& fields = dn_type->get_fields();

    OTHER_ASSERT(num_fields <= fields.size(), "Invalid number of fields!");

    for (uint64_t i = 0; i < num_fields; ++i) {
      uint16_t field_name_len = serialization::read_value<uint16_t>(buffer, cursor);
      std::string name = serialization::read_string_value(buffer, field_name_len, cursor);

      uint8_t type = serialization::read_value<uint8_t>(buffer, cursor);
      uint64_t data_len = serialization::read_value<uint64_t>(buffer, cursor);

      auto storage_itr = load_field(name, (value_type)type);
      OTHER_ASSERT(storage_itr != field_storage.end(), "Failed to load field storage!");

      std::span<const uint8_t> field_blob = buffer.subspan(cursor, data_len);
      cursor += data_len;

      {
        std::stringstream ss;
        for (uint32_t i = 0; i < field_blob.size(); ++i) {
          ss << std::format("{:02X} ", field_blob[i]);
          if ((i + 1) % 16 == 0 && i > 0) {
            ss << "\n";
          }
        }
        std::println("Deserializing field '{}' of type {} with data ({} bytes):\n{}", name, (int)type, field_blob.size(), ss.str());
      }

      storage_itr->second.load_from_bytes(field_blob.data(), field_blob.size());
    }
  }

  std::map<uint64_t, dotnet_field::storage>::iterator dotnet_object::load_field(const std::string_view field_name, value_type type) {
    OTHER_ASSERT(host != nullptr, "dotnet_host is null");
    OTHER_ASSERT(managed_object != nullptr, "Object handle is null");

    auto itr = field_storage.find(FNV(field_name));
    if (itr == field_storage.end()) {
      if (type_has_field(field_name)) {
        bool success = false;
        std::tie(itr, success) = field_storage.emplace(FNV(field_name), dotnet_field::storage{ type, nullptr, 0 });
        OTHER_ASSERT(success, "Failed to create field storage for field '{}'", field_name);
      } else {
        OTHER_ASSERT(false, "Failed to find field '{}' on type '{}'", field_name, get_type_name());
      }
    }
    OTHER_ASSERT(itr != field_storage.end(), "Field storage for '{}' not found", field_name);

    if (itr->second.data == nullptr) {
      itr->second.stored_type = type;
      if (itr->second.stored_type == value_type::STRING) {
        itr->second.size = managed_strlen(field_name) + 1;
      } else {
        itr->second.size = get_value_type_size(itr->second.stored_type);
        itr->second.data = (uint8_t*)arena::allocate(itr->second.size);
      }

      load_field_into_storage(field_name, itr->second);
      if (itr->second.stored_type == value_type::STRING) {
        /// add null terminator for string types
        itr->second.data[itr->second.size - 1] = '\0';
      }
    }
    return itr;
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
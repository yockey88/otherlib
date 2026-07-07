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
#include "script/scripting_environment.hpp"

namespace other {

  void dotnet_object::load_fields() {
    OTHER_ASSERT(host != nullptr, "dotnet_host is null");
    OTHER_ASSERT(managed_object != nullptr, "Object handle is null");

    const auto& fields = dn_type->get_fields();
    for (const auto& f : fields) {
      // skip C# property backing fields
      if (f.name().ends_with("k__BackingField")) {
        continue;
      }

      /// \todo find a way to deserialize the user types into storage
      ///        class SerializedAttribute : Attribute {}
      ///        [Serialized]
      if (f.get_type() == value_type::USER_TYPE) {
        continue;
      }

      auto itr = load_field(f.name(), f.get_type());
      OTHER_ASSERT(itr != field_storage.end(), "Failed to load field storage for field '{}'", f.name());
      CORE_LOG_DEBUG("Loading field '{}' of type '{}' for dotnet object '{}'", f.name(), itr->second.stored_type, object_name);
    }
  }

  void dotnet_object::write_fields() {
    OTHER_ASSERT(host != nullptr, "dotnet_host is null");
    OTHER_ASSERT(managed_object != nullptr, "Object handle is null");

    const auto& fields = dn_type->get_fields();
    for (const auto& f : fields) {
      // skip C# property backing fields
      if (f.name().ends_with("k__BackingField")) {
        continue;
      }

      /// \todo find a way to serialize the user types from storage
      ///        class SerializedAttribute : Attribute {}
      ///        [Serialized]
      if (f.get_type() == value_type::USER_TYPE) {
        continue;
      }

      auto itr = std::ranges::find_if(field_storage, [&](const auto& pair) {
        return pair.first == FNV(f.name());
      });
      OTHER_ASSERT(itr != field_storage.end(), "Failed to find field storage for field '{}'", f.name());

      write_storage_to_field(itr, f.name());
    }
  }

  behavior_snapshot dotnet_object::get_behavior_snapshot() const {
    // auto* env = subsystem<scripting_environment>::get();
    // OTHER_ASSERT(env != nullptr, "Scripting environment is not initialized.");

    // auto& fns = env->get_dotnet_host().interop();

    // behavior_snapshot snapshot{};

    // int32_t behavior_count = fns.get_behavior_count(managed_object);
    // if (behavior_count <= 0) {
    //   snapshot.valid = true;
    //   return snapshot;
    // }

    // snapshot.behaviors.reserve(behavior_count);

    // for (int32_t bi = 0; bi < behavior_count; ++bi) {
    //   behavior_descriptor desc{};
    //   desc.behavior_index = bi;

    //   // type name
    //   {
    //     native_string name = fns.get_behavior_type_name(managed_object, bi);
    //     desc.full_type_name = std::string(name);
    //     native_string::free_str(name);
    //   }

    //   // display name
    //   {
    //     native_string name = fns.get_behavior_display_name(managed_object, bi);
    //     desc.display_name = std::string(name);
    //     native_string::free_str(name);
    //   }

    //   // fields
    //   int32_t field_count = fns.get_behavior_field_count(managed_object, bi);
    //   desc.fields.reserve(field_count);

    //   for (int32_t fi = 0; fi < field_count; ++fi) {
    //     behavior_field_descriptor field_desc{};
    //     field_desc.field_index = fi;

    //     // field name
    //     {
    //       native_string name = fns.get_behavior_field_name(managed_object, bi, fi);
    //       field_desc.field_name = std::string(name);
    //       native_string::free_str(name);
    //     }

    //     // display name
    //     {
    //       native_string name = fns.get_behavior_field_display_name(managed_object, bi, fi);
    //       field_desc.display_name = std::string(name);
    //       native_string::free_str(name);
    //     }

    //     // descriptor (type, flags, range)
    //     {
    //       native_behavior_field_descriptor native_desc{};
    //       fns.get_behavior_field_descriptor(managed_object, bi, fi, &native_desc);
    //       field_desc.type = static_cast<value_type>(native_desc.field_value_type);
    //       field_desc.flags = static_cast<behavior_display_flags>(native_desc.flags);
    //       field_desc.range_min = native_desc.range_min;
    //       field_desc.range_max = native_desc.range_max;
    //     }

    //     // tooltip
    //     if (has_flag(field_desc.flags, behavior_display_flags::has_tooltip)) {
    //       native_string tip = fns.get_behavior_field_tooltip(managed_object, bi, fi);
    //       field_desc.tooltip = std::string(tip);
    //       native_string::free_str(tip);
    //     }

    //     // group name
    //     if (has_flag(field_desc.flags, behavior_display_flags::is_group_start)) {
    //       native_string group = fns.get_behavior_field_group_name(managed_object, bi, fi);
    //       field_desc.group_name = std::string(group);
    //       native_string::free_str(group);
    //     }

    //     desc.fields.push_back(std::move(field_desc));
    //   }

    //   snapshot.behaviors.push_back(std::move(desc));
    // }

    // snapshot.valid = true;
    return {};
  }

  bool dotnet_object::has_method(const std::string_view method_name) const {
    OTHER_ASSERT(dn_type != nullptr, "Type is not initialized for dotnet_object '{}'", object_name);
    return dn_type->has_method(method_name);
  }

  int32_t dotnet_object::read_behavior_field_value(int32_t behavior_index, int32_t field_index, void* out_data, int32_t buffer_size) {
    if (managed_object == nullptr) {
      return 0;
    }
    auto* env = subsystem<scripting_environment>::get();
    OTHER_ASSERT(env != nullptr, "Scripting environment is not initialized.");

    int32_t bytes_written = 0;
    // env->get_dotnet_host().interop().get_behavior_field_value(managed_object, behavior_index, field_index, out_data, &bytes_written);
    return bytes_written;
  }

  bool dotnet_object::write_field_value(int32_t behavior_index, int32_t field_index, void* in_data, int32_t data_size) {
    if (managed_object == nullptr) {
      return false;
    }
    auto* env = subsystem<scripting_environment>::get();
    OTHER_ASSERT(env != nullptr, "Scripting environment is not initialized.");

    // env->get_dotnet_host().interop().set_behavior_field_value(managed_object, behavior_index, field_index, in_data, data_size);
    return true;
  }

  std::string dotnet_object::get_type_name() const {
    OTHER_ASSERT(dn_type != nullptr, "Type is not initialized for dotnet_object '{}'", object_name);
    return dn_type->full_name();
  }

  bool dotnet_object::has_attribute(const std::string_view attr_name) {
    OTHER_ASSERT(host != nullptr, "dotnet_host is null");
    return dn_type->has_attribute(attr_name);
  }

  ostd::vector<std::string> dotnet_object::get_attribute_names() const {
    return dn_type->get_attribute_names();
  }

  namespace detail {

    static bool is_dotnet_builtin(const std::string_view name) {
      return name.ends_with("k__BackingField");
    }

  }  // namespace detail

  ostd::vector<uint8_t> dotnet_object::serialize_to_bytes() {
    OTHER_ASSERT(host != nullptr, "dotnet_host is null");
    OTHER_ASSERT(managed_object != nullptr, "Object handle is null");

    ostd::vector<uint8_t> bytes = {};

    /**
     | num fields | fields |
     |---------------------|
     | 2 bytes    |        |

    fields:
     | field name len | field name | field type | field data len | field data |
     |------------------------------------------------------------------------|
     | 2 byte         |            | 1 byte     | 8 bytes        |            |
    **/

    // const auto& fields = dn_type->get_fields();

    // size_t num_fields = std::ranges::count_if(fields, [](const dotnet_field& f) { return !detail::is_dotnet_builtin(f.name()); });
    // serialization::write_value<uint16_t>((uint16_t)num_fields, bytes);

    // for (const auto& f : fields) {
    //   if (detail::is_dotnet_builtin(f.name())) {
    //     continue;
    //   }

    //   auto storage_itr = load_field(f.name(), f.get_type());
    //   OTHER_ASSERT(storage_itr != field_storage.end(), "Failed to load field storage for field '{}'", f.name());
    //   serialization::write_value<uint16_t>((uint16_t)f.name().size(), bytes);
    //   serialization::write_string_value(f.name(), bytes);
    //   serialization::write_value<uint8_t>((uint8_t)storage_itr->second.stored_type, bytes);
    //   serialization::write_value<uint64_t>(storage_itr->second.size, bytes);
    //   serialization::write_bytes(storage_itr->second.data, storage_itr->second.size, bytes);
    // }

    return bytes;
  }

  void dotnet_object::load_from_bytes(const std::span<const uint8_t> buffer) {
    OTHER_ASSERT(host != nullptr, "dotnet_host is null!");
    OTHER_ASSERT(managed_object != nullptr, "Object handle is null!");

    // uint64_t cursor = 0;

    // uint16_t num_fields = serialization::read_value<uint16_t>(buffer, cursor);
    // const auto& fields = dn_type->get_fields();

    // OTHER_ASSERT(num_fields == fields.size() - std::ranges::count_if(fields, [](const dotnet_field& f) { return detail::is_dotnet_builtin(f.name()); }), "Invalid number of fields!");

    // for (uint64_t i = 0; i < num_fields; ++i) {
    //   uint16_t field_name_len = serialization::read_value<uint16_t>(buffer, cursor);
    //   std::string name = serialization::read_string_value(buffer, field_name_len, cursor);

    //   uint8_t type = serialization::read_value<uint8_t>(buffer, cursor);
    //   uint64_t data_len = serialization::read_value<uint64_t>(buffer, cursor);

    //   auto storage_itr = load_field(name, (value_type)type);
    //   OTHER_ASSERT(storage_itr != field_storage.end(), "Failed to load field storage!");

    //   std::span<const uint8_t> field_blob = buffer.subspan(cursor, data_len);
    //   cursor += data_len;

    //   storage_itr->second.load_from_bytes(field_blob.data(), field_blob.size());
    //   write_storage_to_field(storage_itr, name);
    // }
  }

  ostd::vector<uint8_t> dotnet_object::serialize_field_to_bytes(const std::string_view name) {
    // const auto& fields = dn_type->get_fields();
    // auto itr = std::ranges::find_if(fields, [&](const dotnet_field& f) { return f.name() == name; });

    ostd::vector<uint8_t> bytes = {};
    // if (itr == fields.end()) {
    //   CORE_LOG_ERROR("Failed to find field {} on type {}", name, dn_type->full_name());
    //   return bytes;
    // }

    // auto storage_itr = load_field(itr->name(), itr->get_type());
    // OTHER_ASSERT(storage_itr != field_storage.end(), "Failed to load field storage for field '{}'", itr->name());
    // serialization::write_value<uint16_t>((uint16_t)itr->name().size(), bytes);
    // serialization::write_string_value(itr->name(), bytes);
    // serialization::write_value<uint8_t>((uint8_t)storage_itr->second.stored_type, bytes);
    // serialization::write_value<uint64_t>(storage_itr->second.size, bytes);
    // serialization::write_bytes(storage_itr->second.data, storage_itr->second.size, bytes);

    return bytes;
  }

  bool dotnet_object::is_behavior() const {
    OTHER_ASSERT(dn_type != nullptr, "Type is not initialized for dotnet_object '{}'", object_name);
    auto* env = subsystem<scripting_environment>::get();
    OTHER_ASSERT(env != nullptr, "Scripting environment is not initialized.");

    int32_t behavior_base_type_id = env->get_dotnet_host().get_behavior_base_type_id();
    return host->interop().derived_from(dn_type->dotnet_id, behavior_base_type_id);
  }

  const dotnet_field* dotnet_object::get_dotnet_field(const std::string_view field_name) {
    OTHER_ASSERT(dn_type != nullptr, "Type is not initialized for dotnet_object '{}'", object_name);

    auto itr = std::ranges::find_if(dn_type->get_fields(), [&](const dotnet_field& f) { return f.name() == field_name; });
    if (itr == dn_type->get_fields().end()) {
      return nullptr;
    }
    return &(*itr);
  }

  dotnet_field::storage& dotnet_object::get_field_storage(const std::string_view field_name) {
    auto itr = field_storage.find(FNV(field_name));
    OTHER_ASSERT(itr != field_storage.end(), "Field '{}' not found in storage", field_name);
    return itr->second;
  }

  const dotnet_field::storage& dotnet_object::get_field_storage(const std::string_view field_name) const {
    auto itr = field_storage.find(FNV(field_name));
    OTHER_ASSERT(itr != field_storage.end(), "Field '{}' not found in storage", field_name);
    return itr->second;
  }

  void dotnet_object::set_field(const std::string_view field_name, const value& val) {
    if (!type_has_field(field_name)) {
      CORE_LOG_ERROR("Field '{}' not found on .NET type {}", field_name, dn_type->full_name());
      return;
    }
    if (val.type() != get_field_type(field_name)) {
      CORE_LOG_ERROR("Type mismatch when setting field '{}' on .NET type {}: expected {}, got {}", field_name, dn_type->full_name(), get_field_type(field_name), val.type());
      return;
    }

    switch (get_field_type(field_name)) {
      case value_type::CHAR: set_field<char>(field_name, (char)val); break;
      case value_type::OEBOOL: set_field<bool>(field_name, (bool)val); break;
      case value_type::INT8: set_field<int8_t>(field_name, (int8_t)val); break;
      case value_type::INT16: set_field<int16_t>(field_name, (int16_t)val); break;
      case value_type::INT32: set_field<int32_t>(field_name, (int32_t)val); break;
      case value_type::INT64: set_field<int64_t>(field_name, (int64_t)val); break;
      case value_type::UINT8: set_field<uint8_t>(field_name, (uint8_t)val); break;
      case value_type::UINT16: set_field<uint16_t>(field_name, (uint16_t)val); break;
      case value_type::UINT32: set_field<uint32_t>(field_name, (uint32_t)val); break;
      case value_type::UINT64: set_field<uint64_t>(field_name, (uint64_t)val); break;
      case value_type::FLOAT: set_field<float>(field_name, (float)val); break;
      case value_type::DOUBLE: set_field<double>(field_name, (double)val); break;
      case value_type::STRING: set_field<std::string>(field_name, (std::string)val); break;
      default:
        CORE_LOG_ERROR(" - Unsupported field type [{}] for field '{}' in .NET type {}", val.type(), field_name, get_type_name());
        break;
    }
  }

  value_type dotnet_object::get_field_type(const std::string_view field_name) {
    auto itr = load_field<value>(field_name);
    if (itr == field_storage.end()) {
      CORE_LOG_ERROR("Field '{}' not found on .NET type {}", field_name, dn_type->full_name());
      return value_type::EMPTY_TYPE;
    }

    return itr->second.stored_type;
  }

  ostd::map<uint64_t, dotnet_field::storage>::iterator dotnet_object::load_field(const std::string_view field_name, value_type type) {
    OTHER_ASSERT(host != nullptr, "dotnet_host is null");
    OTHER_ASSERT(managed_object != nullptr, "Object handle is null");

    CORE_LOG_TRACE("Loading field '{}' of type '{}' for dotnet object '{}'", field_name, type, object_name);
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

  void dotnet_object::write_storage_to_field(ostd::map<uint64_t, dotnet_field::storage>::const_iterator itr, const std::string_view field_name) {
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
    host->interop().invoke_instance_method(managed_object, name, argv, arg_ts, argc);
    native_string::free_str(name);
  }

  void dotnet_object::invoke_returning_method_args(const std::string_view method_name, const void** argv, const managed_type* arg_ts, size_t argc, void* out) {
    OTHER_ASSERT(host != nullptr, "dotnet_host is null");
    auto name = native_string::new_str(method_name);
    host->interop().invoke_instance_method_ret(managed_object, name, argv, arg_ts, argc, out);
    native_string::free_str(name);
  }

}  // namespace other
/**
 * \file dotnet/dotnet_object.cpp
 **/
#include "dotnet/dotnet_object.hpp"

#include <algorithm>
#include <cstring>
#include <print>
#include <string>

#include "core/fnv.hpp"
#include "serialization/serialization.hpp"

#include "dotnet/dotnet_host.hpp"
#include "dotnet/native_string.hpp"
#include "script/scripting_environment.hpp"

namespace other {

  void dotnet_object::load_fields() {
    OTHER_ASSERT(host != nullptr, "dotnet_host is null");
    OTHER_ASSERT(managed_object != nullptr, "Object handle is null");

    const auto& fields = dn_type->get_fields();
    for (const auto& f : fields) {
      if (!is_eagerly_loadable(f)) {
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
      if (!is_eagerly_loadable(f)) {
        continue;
      }

      auto itr = std::ranges::find_if(field_storage, [&](const auto& pair) {
        return pair.first == FNV(f.name());
      });
      OTHER_ASSERT(itr != field_storage.end(), "Failed to find field storage for field '{}'", f.name());

      write_storage_to_field(itr, f.name());
    }
  }

  /// eager sweeps cover plain data fields only. properties stay lazy, by name: getters can
  ///  run arbitrary code (SceneObject.WorldMatrix calls back into native), so evaluating
  ///  them wholesale at attach/serialize time is never safe. user types are unrepresentable
  ///  in flat storage until a [Serialized] story exists
  bool dotnet_object::is_eagerly_loadable(const dotnet_field& f) const {
    if (f.is_property() || f.name().ends_with("k__BackingField")) {
      return false;
    }
    const value_type ft = f.get_type();
    return ft != value_type::USER_TYPE && ft != value_type::EMPTY_TYPE;
  }

  bool dotnet_object::has_method(const std::string_view method_name) const {
    OTHER_ASSERT(dn_type != nullptr, "Type is not initialized for dotnet_object '{}'", object_name);
    return dn_type->has_method(method_name);
  }

  int32_t dotnet_object::read_field_value(const std::string_view field_name, void* out_data, int32_t buffer_size) {
    if (managed_object == nullptr || dn_type == nullptr || out_data == nullptr || buffer_size <= 0) {
      return 0;
    }

    const dotnet_field* f = get_dotnet_field(field_name);
    if (f == nullptr) {
      return 0;
    }
    const value_type ft = f->get_type();
    if (ft == value_type::EMPTY_TYPE || ft == value_type::USER_TYPE) {
      return 0;
    }

    auto itr = load_field(field_name, ft);
    if (itr == field_storage.end()) {
      return 0;
    }

    dotnet_field::storage& storage = itr->second;
    if (storage.stored_type == value_type::STRING) {
      /// strings re-measure on every read; the arena block regrows only when the managed
      ///  string outgrew it
      native_string name = native_string::new_str(field_name);
      native_string str_native;
      if (dn_type->is_field_property(field_name)) {
        host->interop().get_string_property(managed_object, name, &str_native);
      } else {
        host->interop().get_string_field(managed_object, name, &str_native);
      }
      std::string str = str_native;
      native_string::free_str(str_native);
      native_string::free_str(name);

      const size_t needed = str.size() + 1;
      if (needed > storage.size || storage.data == nullptr) {
        arena::free(storage.data);
        storage.data = (uint8_t*)arena::allocate(needed);
      }
      storage.size = needed;
      std::memcpy(storage.data, str.data(), str.size());
      storage.data[needed - 1] = '\0';
    } else {
      load_field_into_storage(field_name, storage);
    }

    const int32_t written = (int32_t)std::min<size_t>(storage.size, (size_t)buffer_size);
    std::memcpy(out_data, storage.data, written);
    return written;
  }

  bool dotnet_object::write_field_value(const std::string_view field_name, const void* in_data, int32_t data_size) {
    if (managed_object == nullptr || dn_type == nullptr || in_data == nullptr || data_size <= 0) {
      return false;
    }

    const dotnet_field* f = get_dotnet_field(field_name);
    if (f == nullptr) {
      return false;
    }

    const auto write_as = [&]<typename FT>() -> bool {
      if ((size_t)data_size < sizeof(FT)) {
        CORE_LOG_ERROR("write_field_value: buffer too small for field '{}' ({} < {})", field_name, data_size, sizeof(FT));
        return false;
      }
      FT v{};
      std::memcpy(&v, in_data, sizeof(FT));
      set_field<FT>(field_name, v);
      return true;
    };

    switch (f->get_type()) {
      case value_type::CHAR: return write_as.operator()<char>();
      case value_type::OEBOOL: return write_as.operator()<bool>();
      case value_type::INT8: return write_as.operator()<int8_t>();
      case value_type::INT16: return write_as.operator()<int16_t>();
      case value_type::INT32: return write_as.operator()<int32_t>();
      case value_type::INT64: return write_as.operator()<int64_t>();
      case value_type::UINT8: return write_as.operator()<uint8_t>();
      case value_type::UINT16: return write_as.operator()<uint16_t>();
      case value_type::UINT32: return write_as.operator()<uint32_t>();
      case value_type::UINT64: return write_as.operator()<uint64_t>();
      case value_type::FLOAT: return write_as.operator()<float>();
      case value_type::DOUBLE: return write_as.operator()<double>();
      case value_type::STRING: {
        /// data is a null-terminated buffer from the widget; trust data_size as the cap
        const char* chars = reinterpret_cast<const char*>(in_data);
        const size_t len = strnlen(chars, (size_t)data_size);
        set_field<std::string>(field_name, std::string(chars, len));
        return true;
      }
      default:
        CORE_LOG_ERROR("write_field_value: unsupported field type [{}] for field '{}'", f->get_type(), field_name);
        return false;
    }
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
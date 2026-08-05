/**
 * \file dotnet/dotnet_object.hpp
 **/
#ifndef OTHER_SCRIPTING_DOTNET_DOTNET_OBJECT_HPP
#define OTHER_SCRIPTING_DOTNET_DOTNET_OBJECT_HPP

#include <concepts>
#include <map>
#include <string>
#include <type_traits>

#include "core/defines.hpp"
#include "core/fnv.hpp"
#include "core/value.hpp"
#include "core/value_type_size.hpp"
#include "memory/arena.hpp"

#include "dotnet/behavior_descriptor.hpp"
#include "dotnet/dotnet_field.hpp"
#include "dotnet/dotnet_type.hpp"
#include "dotnet/types.hpp"

namespace other {

  class dotnet_host;

  class dotnet_object {
   public:
    dotnet_object(dotnet_host* host)
        : host(host) {}
    ~dotnet_object() {}

    void load_fields();
    void write_fields();

    behavior_snapshot get_behavior_snapshot() const;

    bool has_method(const std::string_view method_name) const;

    int32_t read_behavior_field_value(int32_t behavior_index, int32_t field_index, void* out_data, int32_t buffer_size);
    bool write_field_value(int32_t behavior_index, int32_t field_index, void* in_data, int32_t data_size);

    std::string get_type_name() const;

    bool has_attribute(const std::string_view attr_name);
    ostd::vector<std::string> get_attribute_names() const;

    ostd::vector<uint8_t> serialize_to_bytes();
    void load_from_bytes(const std::span<const uint8_t> buffer);

    ostd::vector<uint8_t> serialize_field_to_bytes(const std::string_view name);

    template <typename T>
    T get_attribute(const std::string_view attr_name, const std::string_view field_name) {
      OTHER_ASSERT(host != nullptr, "dotnet_host is null");
      OTHER_ASSERT(managed_object != nullptr, "Object handle is null");

      return dn_type->get_attribute<T>(attr_name, field_name);
    }

    bool is_behavior() const;

    template <typename R = void, typename... Args>
    R invoke(const std::string_view method_name, Args&&... args) {
      if constexpr (std::same_as<R, void>) {
        invoke_void(method_name, std::forward<Args>(args)...);
      } else {
        return invoke_ret<R>(method_name, std::forward<Args>(args)...);
      }
    }

    const dotnet_field* get_dotnet_field(const std::string_view field_name);
    dotnet_field::storage& get_field_storage(const std::string_view field_name);
    const dotnet_field::storage& get_field_storage(const std::string_view field_name) const;

    void set_field(const std::string_view field_name, const value& val);
    value_type get_field_type(const std::string_view field_name);

    template <typename FT>
      requires std::is_copy_constructible_v<FT>
    void set_field(const std::string_view field_name, const FT& value) {
      set_field_or_property<FT>(field_name, value);
    }

    template <typename FT>
      requires std::is_copy_constructible_v<FT>
    FT get_field(const std::string_view field_name) {
      return get_field_or_property<FT>(field_name);
    }

    template <typename FT>
      requires std::is_copy_constructible_v<FT>
    void set_property(const std::string_view property_name, const FT& value) {
      set_field_or_property<FT>(property_name, value);
    }

    template <typename FT>
      requires std::is_copy_constructible_v<FT>
    FT get_property(const std::string_view property_name) {
      return get_field_or_property<FT>(property_name);
    }

    void* managed_object = nullptr;

    std::string object_name = "UnknownDotnetObject";
    dotnet_type* dn_type = nullptr;

   private:
    dotnet_host* host = nullptr;

    ostd::map<uint64_t, dotnet_field::storage> field_storage;

    template <typename FT>
      requires std::is_copy_constructible_v<FT>
    FT get_field_or_property(const std::string_view field_name) {
      auto itr = load_field<FT>(field_name);
      if (itr == field_storage.end()) {
        return FT{};
      } else {
        OTHER_ASSERT(itr->second.data != nullptr, "Field '{}' data is null", field_name);
        OTHER_ASSERT(itr->second.stored_type == get_value_type<FT>(), "Field '{}' type mismatch: expected {}, got {}", field_name, get_value_type<FT>(), itr->second.stored_type);
        return itr->second.template get_as<FT>();
      }
    }

    template <typename FT>
      requires std::is_copy_constructible_v<FT>
    void set_field_or_property(const std::string_view field_name, const FT& value) {
      auto itr = load_field<FT>(field_name);
      if (itr != field_storage.end()) {
        itr->second.template set_as<FT>(value);
      } else {
        CORE_LOG_ERROR("Field '{}' not found", field_name);
        return;
      }
      write_storage_to_field(itr, field_name);
    }

    template <typename FT>
    ostd::map<uint64_t, dotnet_field::storage>::iterator load_field(const std::string_view field_name) {
      OTHER_ASSERT(host != nullptr, "dotnet_host is null");
      OTHER_ASSERT(managed_object != nullptr, "Object handle is null");

      auto itr = field_storage.find(FNV(field_name));
      if (itr == field_storage.end()) {
        if (type_has_field(field_name)) {
          bool success = false;
          std::tie(itr, success) = field_storage.emplace(FNV(field_name), dotnet_field::storage{ get_value_type<FT>(), nullptr, 0 });
          if (!success) {
            CORE_LOG_ERROR("Failed to create field storage for field '{}'", field_name);
            return field_storage.end();
          }
        } else {
          CORE_LOG_ERROR("Field '{}' not found in type!", field_name);
          return field_storage.end();
        }
      }
      OTHER_ASSERT(itr != field_storage.end(), "Field storage for '{}' not found", field_name);

      if (itr->second.data == nullptr) {
        itr->second.stored_type = get_value_type<FT>();
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

    ostd::map<uint64_t, dotnet_field::storage>::iterator load_field(const std::string_view field_name, value_type type);

    void write_storage_to_field(ostd::map<uint64_t, dotnet_field::storage>::const_iterator itr, const std::string_view field_name);

    size_t managed_strlen(const std::string_view field_name);
    bool type_has_field(const std::string_view field_name);
    void load_field_into_storage(const std::string_view field_name, dotnet_field::storage& storage);
    void load_string_field_into_storage(const std::string_view field_name, dotnet_field::storage& storage);

    void invoke_method_with_args(const std::string_view method_name, const void** argv, const managed_type* arg_ts, size_t argc);
    void invoke_returning_method_args(const std::string_view method_name, const void** argv, const managed_type* arg_ts, size_t argc, void* out);

    template <typename... Args>
    void invoke_void(const std::string_view method_name, Args&&... args) {
      constexpr size_t argc = sizeof...(args);
      if constexpr (argc > 0) {
        const void* argv[argc] = {};
        managed_type arg_ts[argc] = {};
        detail::create_opaque_handle_array<Args...>(argv, arg_ts, std::forward<Args>(args)..., std::make_index_sequence<argc>{});
        invoke_method_with_args(method_name, argv, arg_ts, argc);
      } else {
        invoke_method_with_args(method_name, nullptr, nullptr, 0);
      }
    }

    template <typename R, typename... Args>
    R invoke_ret(const std::string_view method_name, Args&&... args) {
      if constexpr (std::is_pointer_v<R> || std::is_trivial_v<R>) {
        return invoke_trivial_pointer_ret<R, Args...>(method_name, std::forward<Args>(args)...);
      } else if constexpr (is_stringlike_type<R>) {
        return invoke_stringlike_ret<R, Args...>(method_name, std::forward<Args>(args)...);
      } else {
        static_assert(false, "Unsupported return type for invoke_ret");
      }
    }

    template <typename R, typename... Args>
    R invoke_trivial_pointer_ret(const std::string_view method_name, Args&&... args) {
      constexpr size_t argc = sizeof...(args);
      R ret{};
      if constexpr (argc > 0) {
        const void* argv[argc] = {};
        managed_type arg_ts[argc] = {};
        detail::create_opaque_handle_array<Args...>(argv, arg_ts, std::forward<Args>(args)..., std::make_index_sequence<argc>{});
        if constexpr (std::is_pointer_v<R>) {
          invoke_returning_method_args(method_name, argv, arg_ts, argc, (void*)ret);
        } else {
          invoke_returning_method_args(method_name, argv, arg_ts, argc, &ret);
        }
      } else {
        if constexpr (std::is_pointer_v<R>) {
          invoke_returning_method_args(method_name, nullptr, nullptr, 0, (void*)ret);
        } else {
          invoke_returning_method_args(method_name, nullptr, nullptr, 0, &ret);
        }
      }
      return std::move(ret);
    }

    template <typename R, typename... Args>
    R invoke_stringlike_ret(const std::string_view method_name, Args&&... args) {
      native_string ret_str = native_string::new_str("");
      constexpr size_t argc = sizeof...(args);
      if constexpr (argc > 0) {
        const void* argv[argc] = {};
        managed_type arg_ts[argc] = {};
        detail::create_opaque_handle_array<Args...>(argv, arg_ts, std::forward<Args>(args)..., std::make_index_sequence<argc>{});
        invoke_returning_method_args(method_name, argv, arg_ts, argc, &ret_str);
      } else {
        invoke_returning_method_args(method_name, nullptr, nullptr, 0, &ret_str);
      }

      std::string ret = ret_str;
      native_string::free_str(ret_str);
      return ret;
    }
  };

}  // namespace other

#endif  // OTHER_SCRIPTING_DOTNET_DOTNET_OBJECT_HPP
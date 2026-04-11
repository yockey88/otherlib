/**
 * \file script/scripting_environment.hpp
 **/
#ifndef OTHER_SCRIPTING_SCRIPT_SCRIPTING_ENVIRONMENT_HPP
#define OTHER_SCRIPTING_SCRIPT_SCRIPTING_ENVIRONMENT_HPP

#include "core/memory_pool.hpp"
#include "core/scope.hpp"
#include "core/subsystem.hpp"

#include "dotnet/dotnet_object.hpp"
#include "dotnet/host.hpp"
#include "lua/lua_host.hpp"
#include "python/interpreter.hpp"
#include "script/script_object.hpp"

namespace other {

  class scripting_environment : public subsystem<scripting_environment> {
   public:
    scripting_environment() = default;
    virtual ~scripting_environment() = default;

    void initialize_script_environment(const config_table& configuration);
    void shutdown_script_environment();

    integer_t create_object(const std::string_view name);
    void destroy_object(integer_t id);

    void dotnet_register_native_object(integer_t id, const std::string_view type_name);
    void dotnet_unregister_native_object(integer_t id);

    // template <typename T>
    // void register_native_object(string name, T&& value);

    script_object* get_object(integer_t id);

    /// DOTNET
    dotnet_host& get_dotnet_host() { return dotnet; }
    const dotnet_host& get_dotnet_host() const { return dotnet; }

    ref<assembly> load_dotnet_module(const std::string_view module_path);
    ref<assembly> get_dotnet_module(const std::string_view module_name);
    void unload_dotnet_module(ref<assembly> module_id);
    void reset_dotnet_environment();

    bool dotnet_object_has_attribute(integer_t id, const std::string_view attr_name);

    void attach_dotnet_behavior(integer_t parent_id, const std::string_view behavior_name);
    void detach_dotnet_behavior(integer_t parent_id, const std::string_view behavior_name);
    void detach_all_dotnet_behaviors(integer_t parent_id);

    template <typename... Args>
    void attach_dotnet_object(integer_t id, const std::string_view type_name, Args&&... ctor_args) {
      script_object* obj = get_object(id);
      OTHER_ASSERT(obj != nullptr, "Script object with ID {} does not exist.", id);

      // handle already attached object
      if (obj->dotnet_object != nullptr) {
        CORE_LOG_WARN("Script object with ID {} already has a .NET object attached. Detaching previous object.", id);
        detach_dotnet_object(id);
      }

      OTHER_ASSERT(obj->dotnet_object == nullptr, "Script object with ID {} already has a .NET object attached.", id);

      CORE_LOG_DEBUG("[script {}] creating .NET object [{} {}]'", id, type_name, obj->name);
      obj->dotnet_object = dotnet.instantiate_managed_object(type_name, obj->name, std::forward<Args>(ctor_args)...);
      if (obj->dotnet_object != nullptr) {
        dotnet_register_native_object(id, type_name);
        obj->dotnet_object->load_fields();
      } else {
        CORE_LOG_ERROR("Failed to attach .NET object of type {} to script object with ID {}", type_name, id);
      }
    }

    template <typename... Args>
    void attach_serialized_dotnet_object(integer_t id, const std::string_view type_name, const std::span<const uint8_t> buffer, Args&&... ctor_args) {
      attach_dotnet_object(id, type_name, std::forward<Args>(ctor_args)...);

      script_object* obj = get_object(id);
      OTHER_ASSERT(obj != nullptr, "Failed to retrieve script object to deserialize dotnet object!");
      obj->dotnet_object->load_from_bytes(buffer);
    }

    template <typename R = void, typename... Args>
      requires std::is_same_v<R, void> || std::is_pointer_v<R> || std::is_trivial_v<R>
    R call_dotnet_method(integer_t id, const std::string_view function_name, Args&&... ctor_args) {
      script_object* obj = get_object(id);
      OTHER_ASSERT(obj != nullptr, "Script object with ID {} does not exist.", id);
      if (obj->dotnet_object != nullptr) {
        return call_method_impl<R>(obj, function_name, std::forward<Args>(ctor_args)...);
      } else {
        CORE_LOG_ERROR("Script object with ID {} does not have a .NET object attached.", id);
        return default_return_value<R>();
      }
    }

    template <typename FT>
      requires std::is_copy_constructible_v<FT>
    FT get_dotnet_field(integer_t id, const std::string_view field_name) {
      script_object* obj = get_object(id);
      OTHER_ASSERT(obj != nullptr, "Script object with ID {} does not exist.", id);
      if (obj->dotnet_object != nullptr) {
        return obj->dotnet_object->get_field<FT>(field_name);
      } else {
        CORE_LOG_ERROR("Script object with ID {} does not have a .NET object attached.", id);
        return FT{};
      }
    }

    template <typename FT>
      requires std::is_copy_constructible_v<FT>
    void set_dotnet_field(integer_t id, const std::string_view field_name, const FT& value) {
      script_object* obj = get_object(id);
      OTHER_ASSERT(obj != nullptr, "Script object with ID {} does not exist.", id);
      if (obj->dotnet_object != nullptr) {
        obj->dotnet_object->set_field(field_name, value);
      } else {
        CORE_LOG_ERROR("Script object with ID {} does not have a .NET object attached.", id);
      }
    }

    template <typename FT>
      requires std::is_copy_constructible_v<FT>
    FT get_dotnet_property(integer_t id, const std::string_view property_name) {
      script_object* obj = get_object(id);
      OTHER_ASSERT(obj != nullptr, "Script object with ID {} does not exist.", id);
      if (obj->dotnet_object != nullptr) {
        return obj->dotnet_object->get_property<FT>(property_name);
      } else {
        CORE_LOG_ERROR("Script object with ID {} does not have a .NET object attached.", id);
        return FT{};
      }
    }

    template <typename T>
    T get_dotnet_attribute(integer_t id, const std::string_view attr_name, const std::string_view field_name) {
      script_object* obj = get_object(id);
      OTHER_ASSERT(obj != nullptr, "Script object with ID {} does not exist.", id);
      if (obj->dotnet_object != nullptr) {
        return obj->dotnet_object->get_attribute<T>(attr_name, field_name);
      } else {
        CORE_LOG_ERROR("Script object with ID {} does not have a .NET object attached.", id);
        return T{};
      }
    }

    void detach_dotnet_object(integer_t id);
    /// END DOTNET

    /// LUA
    lua_host& get_lua_host() { return lua; }
    lua_script* load_lua_file(const std::string_view file_path);
    /// END LUA

    /// PYTHON
    void attach_python_object(integer_t id, const std::string_view type_name);
    void detach_python_object(integer_t id);
    template <typename R = void, typename... Args>
      requires std::is_same_v<R, void> || std::is_pointer_v<R> || std::is_trivial_v<R>
    R call_python_method(integer_t id, const std::string_view function_name, Args&&... ctor_args) {
      return default_return_value<R>();
    }
    /// END PYTHON

    constexpr static inline size_t kMaxScriptObjects = memory_pool<script_object>::kMaxObjects;
    ref<assembly> dotnet_binding_assembly = nullptr;

   private:
    struct live_script_object {
      size_t index = 0;
      script_object* object = nullptr;
    };

    assembly_context* dotnet_load_context = nullptr;
    dotnet_host dotnet;

    lua_host lua;

    python_interpreter python;

    std::array<live_script_object, kMaxScriptObjects> live_objects = {};
    scope<memory_pool<script_object>> script_object_pool = nullptr;

    template <typename R, typename... Args>
      requires std::is_same_v<R, void>
    void call_method_impl(script_object* object, const std::string_view method_name, Args&&... args) {
      OTHER_ASSERT(object != nullptr, "Script object is null");
      object->dotnet_object->invoke<>(method_name, std::forward<Args>(args)...);
    }

    template <typename R, typename... Args>
      requires std::is_pointer_v<R> || std::is_trivial_v<R>
    R call_method_impl(script_object* object, const std::string_view method_name, Args&&... args) {
      OTHER_ASSERT(object != nullptr, "Script object is null");
      return object->dotnet_object->invoke<R>(method_name, std::forward<Args>(args)...);
    }

    template <typename R>
    R default_return_value() {
      if constexpr (std::is_same_v<R, void>) {
        return;
      } else if constexpr (std::is_pointer_v<R>) {
        return nullptr;
      } else {
        return R{};
      }
    }
  };

}  // namespace other

OTHER_SUBSYSTEM(other::scripting_environment);

#endif  // OTHER_SCRIPTING_SCRIPT_SCRIPTING_ENVIRONMENT_HPP
/**
 * \file script/scripting_environment.cpp
 **/
#include "script/scripting_environment.hpp"

#include "core/logger.hpp"

#include "script/script_object.hpp"

namespace other {

  void scripting_environment::initialize_script_environment() {
    script_object_pool = make_scope<memory_pool<script_object>>();
    OTHER_ASSERT(script_object_pool != nullptr, "Failed to create script object memory pool.");

    std::ranges::fill(live_objects, live_script_object{});

    dotnet.load_host();
    dotnet.call_entry_point();

    dotnet_load_context = dotnet.create_assembly_context("Other-DotNet-Assembly-Context");
    OTHER_ASSERT(dotnet_load_context != nullptr, "Failed to create assembly context for .NET assemblies.");

    python.load_host();
    python.call_entry_point();
  }

  void scripting_environment::shutdown_script_environment() {
    python.unload_host();

    {
      natural_t context_handle = dotnet_load_context ? dotnet_load_context->get_handle() : 0;
      dotnet_load_context = nullptr;
      dotnet.destroy_assembly_context(context_handle);
    }
    dotnet.unload_host();

    OTHER_ASSERT(script_object_pool != nullptr, "Script object memory pool is not initialized.");
    script_object_pool->clear();
    script_object_pool = nullptr;
  }

  integer_t scripting_environment::create_object(const std::string_view name) {
    OTHER_ASSERT(script_object_pool != nullptr, "Script object memory pool is not initialized.");
    auto [obj, idx] = script_object_pool->emplace();

    auto& live_obj = live_objects[idx];
    OTHER_ASSERT(live_obj.object == nullptr, "Script object at index {} is already allocated.", idx);
    live_obj.index = idx;

    obj.id = idx;
    obj.name = name;
    live_obj.object = &obj;
    CORE_LOG_DEBUG("Created script object [{}:{}] with ID {}", name, idx, idx);

    return idx;
  }

  void scripting_environment::destroy_object(integer_t id) {
    OTHER_ASSERT(script_object_pool != nullptr, "Script object memory pool is not initialized.");
    /// may occur if object encountered error during creation/on reload
    /// \todo maybe be stricter about this?
    if (id < 0 || id >= kMaxScriptObjects) {
      return;
    }

    CORE_LOG_DEBUG("Destroying script object with ID {}", id);

    script_object* obj = get_object(id);
    OTHER_ASSERT(obj != nullptr, "Script object with ID {} does not exist.", id);

    if (obj->dotnet_object != nullptr) {
      CORE_LOG_DEBUG(" - Destroying .NET object for script object with ID {}", id);
      detach_dotnet_object(id);
    }
    if (obj->python_object != nullptr) {
      CORE_LOG_DEBUG(" - Destroying Python object for script object with ID {}", id);
      detach_python_object(id);
    }
    // if (obj->lua_object != nullptr) {
    // detach_lua_object(id);
    // }

    script_object_pool->free(id);
    live_objects[id] = live_script_object{};
  }

  script_object* scripting_environment::get_object(integer_t id) {
    OTHER_ASSERT(script_object_pool != nullptr, "Script object memory pool is not initialized.");
    OTHER_ASSERT(id >= 0 && id < kMaxScriptObjects, "Invalid script object ID: {}", id);
    return script_object_pool->at(id);
  }

  ref<assembly> scripting_environment::load_dotnet_module(const std::string_view module_path) {
    OTHER_ASSERT(dotnet_load_context != nullptr, "DotNet load context is not initialized.");
    OTHER_ASSERT(!module_path.empty(), "Module path cannot be empty.");
    if (!std::filesystem::exists(module_path)) {
      CORE_LOG_ERROR("Module file does not exist: {}", module_path);
      return 0;
    }

    ref<assembly> asm_ref = dotnet_load_context->load_assembly(module_path);
    if (asm_ref == nullptr) {
      CORE_LOG_ERROR("Failed to load assembly from path: {}", module_path);
    } else {
      CORE_LOG_DEBUG("Loaded assembly [{}:{}] from path: {}", asm_ref->get_handle(), asm_ref->get_name(), module_path);
    }
    return asm_ref;
  }

  void scripting_environment::unload_dotnet_module(ref<assembly> module) {
    OTHER_ASSERT(dotnet_load_context != nullptr, "DotNet load context is not initialized.");
    dotnet_load_context->unload_assembly(module->get_handle());

    if (dotnet_load_context->num_assemblies() == 0) {
      CORE_LOG_DEBUG("All assemblies unloaded from .NET context [{}:{}]", dotnet_load_context->get_handle(), dotnet_load_context->get_name());
      reset_dotnet_environment();
    }
  }

  void scripting_environment::reset_dotnet_environment() {
    natural_t context_handle = dotnet_load_context ? dotnet_load_context->get_handle() : 0;
    dotnet_load_context = nullptr;
    dotnet.destroy_assembly_context(context_handle);

    dotnet_load_context = dotnet.create_assembly_context("Other-DotNet-Assembly-Context");
    OTHER_ASSERT(dotnet_load_context != nullptr, "Failed to create assembly context for .NET assemblies.");
  }

  bool scripting_environment::dotnet_object_has_attribute(integer_t id, const std::string_view attr_name) {
    script_object* obj = get_object(id);
    OTHER_ASSERT(obj != nullptr, "Script object with ID {} does not exist.", id);
    if (obj->dotnet_object != nullptr) {
      return obj->dotnet_object->has_attribute(attr_name);
    } else {
      CORE_LOG_ERROR("Script object with ID {} does not have a .NET object attached.", id);
      return false;
    }
  }

  void scripting_environment::detach_dotnet_object(integer_t id) {
    script_object* obj = get_object(id);
    OTHER_ASSERT(obj != nullptr, "Script object with ID {} does not exist.", id);

    if (obj->dotnet_object == nullptr) {
      CORE_LOG_ERROR("Script object with ID {} does not have a .NET object attached.", id);
      return;
    }

    CORE_LOG_DEBUG("[script {}] destroying .NET object [{}]", id, obj->name);
    dotnet.destroy_managed_object(obj->dotnet_object);
    obj->dotnet_object = nullptr;
  }

  void scripting_environment::attach_python_object(integer_t id, const std::string_view type_name) {
    script_object* obj = get_object(id);
    OTHER_ASSERT(obj != nullptr, "Script object with ID {} does not exist.", id);

    if (obj->python_object != nullptr) {
      CORE_LOG_ERROR("Script object with ID {} already has a Python object attached.", id);
      return;
    }

    CORE_LOG_DEBUG("[script {}] creating Python object [{}] of type [{}]", id, obj->name, type_name);
    // obj->python_object = python.create_script_context(type_name);
  }

  void scripting_environment::detach_python_object(integer_t id) {
  }

}  // namespace other
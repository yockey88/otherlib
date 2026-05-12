/**
 * \file script/scripting_environment.cpp
 **/
#include "script/scripting_environment.hpp"

#include "core/logger.hpp"
#include "core/profiler.hpp"

#include "script/script_object.hpp"

namespace other {

  void scripting_environment::initialize_script_environment(const config_table& configuration) {
    PROFILE_SECTION("scripting_environment::initialize-script-environment");
    script_object_pool = make_scope<memory_pool<script_object>>();
    OTHER_ASSERT(script_object_pool != nullptr, "Failed to create script object memory pool.");

    std::ranges::fill(live_objects, live_script_object{});

    dotnet.load_host(configuration);
    dotnet.call_entry_point();

    dotnet_load_context = dotnet.create_assembly_context("Other-DotNet-Assembly-Context");
    OTHER_ASSERT(dotnet_load_context != nullptr, "Failed to create assembly context for .NET assemblies.");

    lua.load_host(configuration);
    lua.call_entry_point();

    python.load_host();
    python.call_entry_point();
  }

  void scripting_environment::destroy_all_objects() {
    for (auto& obj : live_objects) {
      if (obj.object != nullptr) {
        destroy_object(obj.index);
      }
    }
  }

  void scripting_environment::shutdown_script_environment() {
    // clang-format off
    OTHER_ASSERT(std::ranges::all_of(live_objects, [](const live_script_object& obj) { return obj.status == live_script_object::DESTROYED; }), 
                 "Script leaks detected");
    // clang-format on

    python.unload_host();
    {
      if (dotnet_load_context == nullptr) {
        CORE_LOG_ERROR("DotNet load context is not initialized.");
      } else {
        dotnet_load_context->unload_all();
        dotnet.destroy_assembly_context(dotnet_load_context->get_handle());
      }
      dotnet_load_context = nullptr;
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
    live_obj.status = live_script_object::LIVE;
    CORE_LOG_DEBUG(" - created script object [{}:{}] with ID {}", name, idx, idx);

    return idx;
  }

  void scripting_environment::destroy_object(integer_t id) {
    OTHER_ASSERT(script_object_pool != nullptr, "Script object memory pool is not initialized.");
    /// may occur if object encountered error during creation/on reload
    /// \todo maybe be stricter about this?
    if (id < 0 || id >= kMaxScriptObjects) {
      return;
    }

    CORE_LOG_DEBUG(" - destroying script object with ID {}", id);

    script_object* obj = get_object(id);
    OTHER_ASSERT(obj != nullptr, "Script object with ID {} does not exist.", id);

    if (obj->dotnet_object != nullptr) {
      CORE_LOG_DEBUG(" - destroying .NET object for script object with ID {}", id);
      detach_dotnet_object(id);
    }
    if (obj->python_object != nullptr) {
      CORE_LOG_DEBUG(" - destroying Python object for script object with ID {}", id);
      detach_python_object(id);
    }
    // if (obj->lua_object != nullptr) {
    // detach_lua_object(id);
    // }

    script_object_pool->free(id);
    live_objects[id].object = nullptr;
    live_objects[id].status = live_script_object::DESTROYED;
  }

  void scripting_environment::dotnet_register_native_object(integer_t id, const std::string_view type_name) {
    auto* obj = get_object(id);
    OTHER_ASSERT(obj != nullptr, "Script object with ID {} does not exist.", id);

    if (obj->dotnet_object == nullptr) {
      CORE_LOG_ERROR("Script object with ID {} does not have a .NET object attached.", id);
      return;
    }

    native_string type_str = native_string::new_str(type_name);
    get_dotnet_host().interop().attach_native_object(id, obj->dotnet_object, type_str);
    native_string::free_str(type_str);
  }

  void scripting_environment::dotnet_unregister_native_object(integer_t id) {
    auto* obj = get_object(id);
    OTHER_ASSERT(obj != nullptr, "Script object with ID {} does not exist.", id);

    if (obj->dotnet_object == nullptr) {
      CORE_LOG_ERROR("Script object with ID {} does not have a .NET object attached.", id);
      return;
    }

    get_dotnet_host().interop().detach_native_object(id, obj->dotnet_object);
  }

  script_object* scripting_environment::get_object(integer_t id) {
    OTHER_ASSERT(script_object_pool != nullptr, "Script object memory pool is not initialized.");
    OTHER_ASSERT(id >= 0 && id < kMaxScriptObjects, "Invalid script object ID: {}", id);
    return script_object_pool->at(id);
  }

  ref<assembly> scripting_environment::load_dotnet_module(const std::string_view module_path) {
    OTHER_ASSERT(dotnet_load_context != nullptr, "DotNet load context is not initialized.");
    OTHER_ASSERT(!module_path.empty(), "Module path cannot be empty.");

    PROFILE_SECTION("scripting_environment::load-dotnet-module");
    if (!std::filesystem::exists(module_path)) {
      CORE_LOG_ERROR("Module file does not exist: {}", module_path);
      return nullptr;
    }

    ref<assembly> asm_ref = dotnet_load_context->load_assembly(module_path);
    if (asm_ref == nullptr) {
      CORE_LOG_ERROR("Failed to load assembly from path: {}", module_path);
    } else {
      CORE_LOG_DEBUG("Loaded assembly [{}:{}] from path: {}", asm_ref->get_handle(), asm_ref->get_name(), module_path);
    }
    return asm_ref;
  }

  ref<assembly> scripting_environment::get_dotnet_module(const std::string_view module_name) {
    OTHER_ASSERT(dotnet_load_context != nullptr, "DotNet load context is not initialized.");
    OTHER_ASSERT(!module_name.empty(), "Module name cannot be empty.");
    return dotnet_load_context->get_assembly_by_name(module_name);
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

  bool scripting_environment::dotnet_object_is_behavior(integer_t id) {
    script_object* obj = get_object(id);
    OTHER_ASSERT(obj != nullptr, "Script object with ID {} does not exist.", id);
    if (obj->dotnet_object != nullptr) {
      return obj->dotnet_object->is_behavior();
    } else {
      CORE_LOG_ERROR("Script object with ID {} does not have a .NET object attached.", id);
      return false;
    }
  }

  void scripting_environment::attach_dotnet_behavior(integer_t parent_id, const std::string_view behavior_name) {
    PROFILE_SECTION("scripting_environment::attach_dotnet_behavior");

    script_object* parent = get_object(parent_id);
    OTHER_ASSERT(parent != nullptr, "Script object with ID {} does not exist.", parent_id);
    OTHER_ASSERT(parent->dotnet_object != nullptr, "Script object with ID {} does not have a .NET object attached.", parent_id);

    /// \todo check if this is actually a behavior type

    CORE_LOG_DEBUG("[script {}] attaching behavior '{}' to object '{}'", parent_id, behavior_name, parent->name);

    /// create a script_object slot for this behavior so we have a native-side handle
    std::string behavior_obj_name = std::format("{}__behavior__{}", parent->name, behavior_name);
    integer_t behavior_script_id = create_object(behavior_obj_name);

    dotnet_object* behavior_dotnet_obj = dotnet.instantiate_managed_object(behavior_name, behavior_obj_name);
    if (behavior_dotnet_obj == nullptr) {
      CORE_LOG_ERROR("Failed to instantiate .NET behavior object of type '{}' for script object with ID {}", behavior_name, behavior_script_id);
      destroy_object(behavior_script_id);
      return;
    }

    parent->dotnet_object->invoke<>("AddNativeBehavior", behavior_dotnet_obj->managed_object);
    parent->behavior_handles.push_back({
      .type_name = std::string(behavior_name),
      .script_object_id = behavior_script_id,
    });

    CORE_LOG_DEBUG("[script {}] behavior '{}' attached with script_object ID {}", parent_id, behavior_name, behavior_script_id);
  }

  void scripting_environment::detach_dotnet_behavior(integer_t parent_id, const std::string_view behavior_name) {
    PROFILE_SECTION("scripting_environment::detach_dotnet_behavior");

    script_object* parent = get_object(parent_id);
    OTHER_ASSERT(parent != nullptr, "Script object with ID {} does not exist.", parent_id);
    OTHER_ASSERT(parent->dotnet_object != nullptr, "Script object with ID {} does not have a .NET object attached.", parent_id);

    CORE_LOG_DEBUG("[script {}] detaching behavior '{}' from object '{}'", parent_id, behavior_name, parent->name);

    /// find and remove the behavior handle
    auto it = std::find_if(parent->behavior_handles.begin(), parent->behavior_handles.end(), [&behavior_name](const script_object::behavior_handle& handle) {
      return handle.type_name == behavior_name;
    });

    if (it == parent->behavior_handles.end()) {
      CORE_LOG_WARN("Behavior '{}' not found on script object with ID {}", behavior_name, parent_id);
      return;
    }

    /// destroy the behavior's script_object slot
    if (it->script_object_id >= 0) {
      destroy_object(it->script_object_id);
    }

    parent->behavior_handles.erase(it);

    /// invoke RemoveBehavior on the parent SceneObject's managed object
    native_string type_str = native_string::new_str(behavior_name);
    parent->dotnet_object->invoke<>("RemoveBehavior", type_str);
    native_string::free_str(type_str);

    CORE_LOG_DEBUG("[script {}] behavior '{}' detached", parent_id, behavior_name);
  }

  void scripting_environment::detach_all_dotnet_behaviors(integer_t parent_id) {
    PROFILE_SECTION("scripting_environment::detach_all_dotnet_behaviors");

    script_object* parent = get_object(parent_id);
    OTHER_ASSERT(parent != nullptr, "Script object with ID {} does not exist.", parent_id);

    CORE_LOG_DEBUG("[script {}] detaching all behaviors from object '{}'", parent_id, parent->name);

    /// destroy all behavior script_objects in reverse order
    for (auto it = parent->behavior_handles.rbegin(); it != parent->behavior_handles.rend(); ++it) {
      if (it->script_object_id >= 0) {
        destroy_object(it->script_object_id);
      }
    }
    parent->behavior_handles.clear();

    /// invoke RemoveAllBehaviors on the parent SceneObject's managed object
    if (parent->dotnet_object != nullptr) {
      parent->dotnet_object->invoke<>("RemoveAllBehaviors");
    }
  }

  void scripting_environment::invalidate_dotnet_script_objects_of_type(int32_t dotnet_type_id) {
    PROFILE_SECTION("scripting_environment::invalidate_dotnet_script_objects_of_type");
    CORE_LOG_DEBUG("Invalidating script objects with .NET type ID {}", dotnet_type_id);

    for (auto& live_obj : live_objects) {
      if (live_obj.status == live_script_object::LIVE && live_obj.object != nullptr) {
        script_object* obj = live_obj.object;
        if (obj->dotnet_object == nullptr || obj->dotnet_object->dn_type == nullptr) {
          continue;
        }

        // check if behavior and remove behavior if it matches the type being invalidated
        if (obj->dotnet_object->is_behavior() && obj->dotnet_object->dn_type->dotnet_id == dotnet_type_id) {
          CORE_LOG_DEBUG(" - invalidating behavior script object [{}:{}] with ID {} due to matching .NET type ID {}", obj->name, obj->id, obj->id, dotnet_type_id);
          destroy_object(obj->id);
        } else if (obj->dotnet_object->dn_type->dotnet_id == dotnet_type_id) {
          CORE_LOG_DEBUG(" - invalidating script object [{}:{}] with ID {} due to matching .NET type ID {}", obj->name, obj->id, obj->id, dotnet_type_id);
          detach_dotnet_object(obj->id);
        }
      }
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

    if (obj->dotnet_object->has_method("RemoveAllBehaviors")) {
      CORE_LOG_DEBUG("[script {}] invoking RemoveAllBehaviors on .NET object before detaching", id);
      obj->dotnet_object->invoke<>("RemoveAllBehaviors");
    }

    dotnet_unregister_native_object(id);
    dotnet.destroy_managed_object(obj->dotnet_object);
    obj->dotnet_object = nullptr;
  }

  lua_script* scripting_environment::load_lua_file(const std::string_view file_path) {
    return lua.load_file(file_path);
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
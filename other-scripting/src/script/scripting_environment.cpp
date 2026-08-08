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
  }

  void scripting_environment::destroy_all_objects() {
    PROFILE_SECTION("scripting_environment::destroy_all_objects");
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

    PROFILE_SECTION("scripting_environment::shutdown_script_environment");
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
    PROFILE_SECTION("scripting_environment::destroy_object");
    /// may occur if object encountered error during creation/on reload
    /// \todo maybe be stricter about this?
    if (id < 0 || id >= kMaxScriptObjects) {
      return;
    }
    /// stale handles (hot reload, behaviors already detached) may point at freed slots
    if (live_objects[id].status != live_script_object::LIVE) {
      return;
    }

    CORE_LOG_DEBUG(" - destroying script object with ID {}", id);

    script_object* obj = get_object(id);
    OTHER_ASSERT(obj != nullptr, "Script object with ID {} does not exist.", id);

    /// behaviors own their managed objects; destroyed with their parent so managed names are
    ///  released for reuse (scene teardown only destroys the parent's slot)
    for (auto it = obj->behavior_handles.rbegin(); it != obj->behavior_handles.rend(); ++it) {
      if (it->script_object_id >= 0) {
        destroy_object(it->script_object_id);
      }
    }
    obj->behavior_handles.clear();

    if (obj->dotnet_object != nullptr) {
      CORE_LOG_DEBUG(" - destroying .NET object for script object with ID {}", id);
      detach_dotnet_object(id);
    }

    script_object_pool->free(id);
    live_objects[id].object = nullptr;
    live_objects[id].status = live_script_object::DESTROYED;
  }

  void scripting_environment::dotnet_register_native_object(integer_t id, const std::string_view type_name) {
    PROFILE_SECTION("scripting_environment::dotnet_register_native_object");
    auto* obj = get_object(id);
    OTHER_ASSERT(obj != nullptr, "Script object with ID {} does not exist.", id);

    if (obj->dotnet_object == nullptr) {
      CORE_LOG_ERROR("Script object with ID {} does not have a .NET object attached.", id);
      return;
    }

    native_string type_str = native_string::new_str(type_name);
    get_dotnet_host().interop().attach_native_object(id, obj->dotnet_object, type_str);
    native_string::free_str(type_str);
    obj->dotnet_native_registered = true;
  }

  void scripting_environment::dotnet_unregister_native_object(integer_t id) {
    PROFILE_SECTION("scripting_environment::dotnet_unregister_native_object");
    auto* obj = get_object(id);
    OTHER_ASSERT(obj != nullptr, "Script object with ID {} does not exist.", id);

    if (obj->dotnet_object == nullptr) {
      CORE_LOG_ERROR("Script object with ID {} does not have a .NET object attached.", id);
      return;
    }

    get_dotnet_host().interop().detach_native_object(id, obj->dotnet_object);
    obj->dotnet_native_registered = false;
  }

  script_object* scripting_environment::get_object(integer_t id) {
    OTHER_ASSERT(script_object_pool != nullptr, "Script object memory pool is not initialized.");
    OTHER_ASSERT(id >= 0 && id < kMaxScriptObjects, "Invalid script object ID: {}", id);
    return script_object_pool->at(id);
  }

  void scripting_environment::reset_dotnet_object_binding(integer_t id) {
    script_object* obj = get_object(id);
    OTHER_ASSERT(obj != nullptr, "Script object with ID {} does not exist.", id);
    if (obj->dotnet_object == nullptr) {
      return;
    }

    CORE_LOG_DEBUG("[script {}] resetting .NET native binding for [{}]", id, obj->name);
    obj->dotnet_object->invoke<>("ResetNativeHandle");
  }

  void scripting_environment::rebind_dotnet_object(integer_t id, void* native_handle) {
    script_object* obj = get_object(id);
    OTHER_ASSERT(obj != nullptr, "Script object with ID {} does not exist.", id);
    if (obj->dotnet_object == nullptr) {
      return;
    }

    CORE_LOG_DEBUG("[script {}] rebinding .NET object [{}] to a new native handle", id, obj->name);
    obj->dotnet_object->invoke<>("RebindNativeHandle", native_handle);
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

  ref<assembly> scripting_environment::get_dotnet_module_by_asset_path(const filepath& asset_path) {
    OTHER_ASSERT(dotnet_load_context != nullptr, "DotNet load context is not initialized.");
    OTHER_ASSERT(!asset_path.empty(), "Asset path cannot be empty.");
    return dotnet_load_context->get_assembly_by_id(FNV(asset_path.string()));
  }

  void scripting_environment::unload_dotnet_module(ref<assembly> module) {
    OTHER_ASSERT(dotnet_load_context != nullptr, "DotNet load context is not initialized.");
    PROFILE_SECTION("scripting_environment::unload_dotnet_module");
    dotnet_load_context->unload_assembly(module->get_handle());
    dotnet_load_context->remove_assembly(module->get_handle());

    if (dotnet_load_context->num_assemblies() == 0) {
      CORE_LOG_DEBUG("All assemblies unloaded from .NET context [{}:{}]", dotnet_load_context->get_handle(), dotnet_load_context->get_name());
      reset_dotnet_environment();
    }
  }

  void scripting_environment::reset_dotnet_environment() {
    PROFILE_SECTION("scripting_environment::reset_dotnet_environment");
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

    /// re-applying a scene document to a surviving object (play-stop restore) re-adds
    ///  behaviors that never detached — attach is idempotent per type
    auto existing = std::find_if(parent->behavior_handles.begin(), parent->behavior_handles.end(), [&behavior_name](const script_object::behavior_handle& handle) {
      return handle.type_name == behavior_name;
    });
    if (existing != parent->behavior_handles.end()) {
      CORE_LOG_DEBUG("[script {}] behavior '{}' already attached with script_object ID {}", parent_id, behavior_name, existing->script_object_id);
      return;
    }

    /// \todo check if this is actually a behavior type

    CORE_LOG_DEBUG("[script {}] attaching behavior '{}' to object '{}'", parent_id, behavior_name, parent->name);

    integer_t behavior_script_id = instantiate_dotnet_behavior(parent, behavior_name);
    if (behavior_script_id < 0) {
      return;
    }

    parent->behavior_handles.push_back({
      .type_name = std::string(behavior_name),
      .script_object_id = behavior_script_id,
    });

    CORE_LOG_DEBUG("[script {}] behavior '{}' attached with script_object ID {}", parent_id, behavior_name, behavior_script_id);
  }

  integer_t scripting_environment::instantiate_dotnet_behavior(script_object* parent, const std::string_view behavior_name) {
    PROFILE_SECTION("scripting_environment::instantiate_dotnet_behavior");
    /// create a script_object slot for this behavior so we have a native-side handle
    std::string behavior_obj_name = std::format("{}__behavior__{}", parent->name, behavior_name);
    integer_t behavior_script_id = create_object(behavior_obj_name);

    dotnet_object* behavior_dotnet_obj = dotnet.instantiate_managed_object(behavior_name, behavior_obj_name);
    if (behavior_dotnet_obj == nullptr) {
      CORE_LOG_ERROR("Failed to instantiate .NET behavior object of type '{}' for script object with ID {}", behavior_name, behavior_script_id);
      destroy_object(behavior_script_id);
      return -1;
    }

    /// the slot owns the managed object so destroy_object releases the name for reuse
    get_object(behavior_script_id)->dotnet_object = behavior_dotnet_obj;

    parent->dotnet_object->invoke<>("AddNativeBehavior", behavior_dotnet_obj->managed_object);
    return behavior_script_id;
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

    /// behaviors must be removed through the parent, or the native-side-only destroy roots
    ///  the unloading assembly; the handle stays invalid until reattach restores it
    if (dotnet_type* invalidated_type = dotnet.get_type_cache()->get_type(dotnet_type_id); invalidated_type != nullptr) {
      const std::string type_class_name = invalidated_type->class_name();
      const std::string type_full_name = invalidated_type->full_name();
      for (auto& live_obj : live_objects) {
        if (live_obj.status != live_script_object::LIVE || live_obj.object == nullptr) {
          continue;
        }

        script_object* parent = live_obj.object;
        if (parent->dotnet_object == nullptr) {
          continue;
        }

        for (auto& handle : parent->behavior_handles) {
          if (handle.script_object_id < 0 || (handle.type_name != type_class_name && handle.type_name != type_full_name)) {
            continue;
          }

          CORE_LOG_DEBUG(" - invalidating behavior '{}' on object '{}' due to matching .NET type ID {}", handle.type_name, parent->name, dotnet_type_id);
          native_string type_str = native_string::new_str(handle.type_name);
          parent->dotnet_object->invoke<>("RemoveBehavior", type_str);
          native_string::free_str(type_str);

          destroy_object(handle.script_object_id);
          handle.script_object_id = -1;
        }
      }
    }

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

  void scripting_environment::reattach_invalidated_dotnet_behaviors() {
    PROFILE_SECTION("scripting_environment::reattach_invalidated_dotnet_behaviors");

    for (auto& live_obj : live_objects) {
      if (live_obj.status != live_script_object::LIVE || live_obj.object == nullptr) {
        continue;
      }

      script_object* parent = live_obj.object;
      if (parent->dotnet_object == nullptr) {
        continue;
      }

      for (auto& handle : parent->behavior_handles) {
        if (handle.script_object_id >= 0) {
          continue;
        }

        CORE_LOG_DEBUG("[script {}] reattaching behavior '{}' to object '{}'", parent->id, handle.type_name, parent->name);
        /// failure leaves the handle invalid so the next assembly refresh retries it
        ///  (e.g. the behavior class was removed from this build of the assembly)
        handle.script_object_id = instantiate_dotnet_behavior(parent, handle.type_name);
      }
    }
  }

  void scripting_environment::detach_dotnet_object(integer_t id) {
    PROFILE_SECTION("scripting_environment::detach_dotnet_object");
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

    /// behaviors were never attached to the NativeObjectManager; detaching them
    ///  there would throw C#-side
    if (obj->dotnet_native_registered) {
      dotnet_unregister_native_object(id);
    }
    dotnet.destroy_managed_object(obj->dotnet_object);
    obj->dotnet_object = nullptr;
  }

  lua_script* scripting_environment::load_lua_file(const std::string_view file_path) {
    return lua.load_file(file_path);
  }

}  // namespace other
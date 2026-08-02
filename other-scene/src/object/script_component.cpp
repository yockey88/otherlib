/**
 * \file object/script_component.cpp
 **/
#include "object/script_component.hpp"

#include "core/profiler.hpp"

#include "script/scripting_environment.hpp"

namespace other {

  void script_component::fixed_update(double delta_time) {
    OTHER_ASSERT(script_object_id >= 0, "Invalid script object ID: {}", script_object_id);
    PROFILE_SECTION("script_component::fixed_update");

    auto* env = subsystem<scripting_environment>::get();
    OTHER_ASSERT(env != nullptr, "Scripting environment is not initialized.");

    script_object* script_obj = env->get_object(script_object_id);
    OTHER_ASSERT(script_obj != nullptr, "Script object with ID {} not found in scripting environment.", script_object_id);

    if (auto* dn_obj = script_obj->dotnet_object; dn_obj != nullptr) {
      dn_obj->invoke<>("FixedUpdate");
    }
  }

  void script_component::update(double delta_time) {
    OTHER_ASSERT(script_object_id >= 0, "Invalid script object ID: {}", script_object_id);
    PROFILE_SECTION("script_component::update");

    auto* env = subsystem<scripting_environment>::get();
    OTHER_ASSERT(env != nullptr, "Scripting environment is not initialized.");

    script_object* script_obj = env->get_object(script_object_id);
    OTHER_ASSERT(script_obj != nullptr, "Script object with ID {} not found in scripting environment.", script_object_id);

    if (auto* dn_obj = script_obj->dotnet_object; dn_obj != nullptr) {
      dn_obj->invoke<>("Update");
    }
  }

  void script_component::late_update(double delta_time) {
    OTHER_ASSERT(script_object_id >= 0, "Invalid script object ID: {}", script_object_id);
    PROFILE_SECTION("script_component::late_update");
    if (script_object_id < 0) {
      return;
    }

    auto* env = subsystem<scripting_environment>::get();
    OTHER_ASSERT(env != nullptr, "Scripting environment is not initialized.");

    script_object* script_obj = env->get_object(script_object_id);
    OTHER_ASSERT(script_obj != nullptr, "Script object with ID {} not found in scripting environment.", script_object_id);

    if (auto* dn_obj = script_obj->dotnet_object; dn_obj != nullptr) {
      dn_obj->invoke<>("LateUpdate");
    }
  }

  void script_component::render_update(double delta_time) {
    OTHER_ASSERT(script_object_id >= 0, "Invalid script object ID: {}", script_object_id);
    PROFILE_SECTION("script_component::render_update");

    auto* env = subsystem<scripting_environment>::get();
    OTHER_ASSERT(env != nullptr, "Scripting environment is not initialized.");

    script_object* script_obj = env->get_object(script_object_id);
    OTHER_ASSERT(script_obj != nullptr, "Script object with ID {} not found in scripting environment.", script_object_id);

    if (auto* dn_obj = script_obj->dotnet_object; dn_obj != nullptr) {
      dn_obj->invoke<>("Render");
    }
  }

  void script_component::scene_start() {
    OTHER_ASSERT(script_object_id >= 0, "Invalid script object ID: {}", script_object_id);
    PROFILE_SECTION("script_component::scene_start");

    auto* env = subsystem<scripting_environment>::get();
    OTHER_ASSERT(env != nullptr, "Scripting environment is not initialized.");

    script_object* script_obj = env->get_object(script_object_id);
    OTHER_ASSERT(script_obj != nullptr, "Script object with ID {} not found in scripting environment.", script_object_id);

    if (auto* dn_obj = script_obj->dotnet_object; dn_obj != nullptr) {
      dn_obj->invoke<>("SceneStart");
    }
  }

  void script_component::scene_stop() {
    OTHER_ASSERT(script_object_id >= 0, "Invalid script object ID: {}", script_object_id);
    PROFILE_SECTION("script_component::scene_stop");

    auto* env = subsystem<scripting_environment>::get();
    OTHER_ASSERT(env != nullptr, "Scripting environment is not initialized.");

    script_object* script_obj = env->get_object(script_object_id);
    OTHER_ASSERT(script_obj != nullptr, "Script object with ID {} not found in scripting environment.", script_object_id);

    if (auto* dn_obj = script_obj->dotnet_object; dn_obj != nullptr) {
      dn_obj->invoke<>("SceneStop");
    }
  }

  void script_component::reset_dotnet_binding() {
    OTHER_ASSERT(script_object_id >= 0, "Invalid script object ID: {}", script_object_id);
    PROFILE_SECTION("script_component::reset_dotnet_binding");

    auto* env = subsystem<scripting_environment>::get();
    OTHER_ASSERT(env != nullptr, "Scripting environment is not initialized.");

    env->reset_dotnet_object_binding(script_object_id);
  }

  void script_component::add_behavior(const std::string_view behavior_type_name) {
    OTHER_ASSERT(script_object_id >= 0, "Invalid script object ID: {}", script_object_id);
    PROFILE_SECTION("script_component::add_behavior");

    auto* env = subsystem<scripting_environment>::get();
    OTHER_ASSERT(env != nullptr, "Scripting environment is not initialized.");

    env->attach_dotnet_behavior(script_object_id, behavior_type_name);
  }

  void script_component::remove_behavior(const std::string_view behavior_type_name) {
    OTHER_ASSERT(script_object_id >= 0, "Invalid script object ID: {}", script_object_id);
    PROFILE_SECTION("script_component::remove_behavior");

    auto* env = subsystem<scripting_environment>::get();
    OTHER_ASSERT(env != nullptr, "Scripting environment is not initialized.");

    env->detach_dotnet_behavior(script_object_id, behavior_type_name);
  }

  void script_component::remove_all_behaviors() {
    OTHER_ASSERT(script_object_id >= 0, "Invalid script object ID: {}", script_object_id);
    PROFILE_SECTION("script_component::remove_all_behaviors");

    auto* env = subsystem<scripting_environment>::get();
    OTHER_ASSERT(env != nullptr, "Scripting environment is not initialized.");

    /// destroys the behavior script_objects and clears the C# side behavior list
    env->detach_all_dotnet_behaviors(script_object_id);
  }

}  // namespace other
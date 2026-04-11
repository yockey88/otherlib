/**
 * \file scripting/dotnet_bindings/scene_object_bindings.cpp
 **/
#include "scripting/dotnet_bindings/scene_object_bindings.hpp"

#include "script/scripting_environment.hpp"

#include "object/scene_object.hpp"
#include "scene/scene.hpp"

#include "scripting/dotnet_bindings/scene_bindings.hpp"

namespace other {
  namespace bindings {

    void native_get_object_id(void* object_ptr, natural_t* out_id) {
      OTHER_ASSERT(object_ptr != nullptr, "Native object pointer is null.");
      OTHER_ASSERT(out_id != nullptr, "Output ID pointer is null.");

      scripting_environment* env = subsystem<scripting_environment>::get();
      OTHER_ASSERT(env != nullptr, "Scripting environment is not initialized.");

      *out_id = ((scene_object*)object_ptr)->id;
    }

    // void native_get_component(void* object_ptr, int32_t type_handle, void** out_component_ptr) {
    //   OTHER_ASSERT(object_ptr != nullptr, "Native object pointer is null.");
    //   OTHER_ASSERT(out_component_ptr != nullptr, "Output component pointer is null.");

    //   scripting_environment* env = subsystem<scripting_environment>::get();
    //   OTHER_ASSERT(env != nullptr, "Scripting environment is not initialized.");

    //   scene_object* obj = (scene_object*)object_ptr;
    //   // const std::type_info* type_info = env->get_type_info_from_handle(type_handle);
    //   // if (type_info == nullptr) {
    //   //   CORE_LOG_ERROR("Type handle {} does not correspond to a valid type.", type_handle);
    //   //   *out_component_ptr = nullptr;
    //   //   return;
    //   // }

    //   // *out_component_ptr = obj->get_component_by_type(*type_info);
    // }

    void native_component_add_by_name(integer_t id, native_string component_name) {
      scene* active_scene = detail::get_active_scene_checked();
      if (active_scene == nullptr) return;
      scene_object& obj = active_scene->get_object(id);
      active_scene->add_component_by_name(&obj, (std::string)component_name);
    }

    void native_component_remove_by_name(integer_t id, native_string component_name) {
      scene* active_scene = detail::get_active_scene_checked();
      if (active_scene == nullptr) return;
      scene_object& obj = active_scene->get_object(id);
      active_scene->remove_component_by_name(&obj, (std::string)component_name);
    }

    nbool32 native_component_has_by_name(integer_t id, native_string component_name) {
      scene* active_scene = detail::get_active_scene_checked();
      if (active_scene == nullptr) return false;
      scene_object& obj = active_scene->get_object(id);
      return active_scene->has_component_by_name(&obj, (std::string)component_name);
    }

  }  // namespace bindings
}  // namespace other
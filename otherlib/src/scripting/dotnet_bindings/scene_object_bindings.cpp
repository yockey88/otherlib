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

      scene_object* obj = (scene_object*)object_ptr;
      *out_id = obj->id;
    }

    void native_validate_object_handle(natural_t scene_id, natural_t object_id, int32_t generation, nbool32* out_is_valid) {
      OTHER_ASSERT(out_is_valid != nullptr, "Output validity pointer is null.");

      scene* active_scene = detail::get_active_scene_checked();
      if (active_scene == nullptr) {
        *out_is_valid = false;
        return;
      }

      scene_object_handle handle{ (natural_t)scene_id, (natural_t)object_id, (uint32_t)generation };
      *out_is_valid = validate_handle(active_scene, handle);
    }

  }  // namespace bindings
}  // namespace other
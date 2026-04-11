/**
 * \file scripting/dotnet_bindings/scene_bindings.hpp
 **/
#ifndef OTHER_ENVIRONMENT_SCRIPTING_DOTNET_BINDINGS_SCENE_BINDINGS_HPP
#define OTHER_ENVIRONMENT_SCRIPTING_DOTNET_BINDINGS_SCENE_BINDINGS_HPP

#include "core/defines.hpp"

#include "dotnet/native_string.hpp"
#include "dotnet/types.hpp"

namespace other {

  class scene;

  namespace detail {

    scene* get_active_scene_checked();

  }  // namespace detail
  namespace bindings {

    natural_t native_scene_create_object(native_string name, float x, float y, float z);
    void native_scene_destroy_object(natural_t id);
    nbool32 native_scene_has_object(natural_t id);
    native_string native_scene_get_object_name(natural_t id);
    void native_scene_set_object_name(natural_t id, native_string name);
    void native_scene_get_object_ids(natural_t* out_ids, int32_t* out_count, int32_t max_count);
    natural_t native_scene_get_object_count();
    natural_t native_scene_find_object_by_name(native_string name);

    natural_t native_scene_get_parent_id(natural_t id);
    void native_scene_get_children_ids(natural_t id, natural_t* out_ids, int32_t* out_count, int32_t max_count);

    nbool32 native_scene_object_has_tag(natural_t id, native_string tag);
    void native_scene_add_object_tag(natural_t id, native_string tag);
    void native_scene_remove_object_tag(natural_t id, native_string tag);

    nbool32 native_scene_get_object_visible(natural_t id);
    void native_scene_set_object_visible(natural_t id, nbool32 visible);

  }  // namespace bindings
}  // namespace other

#endif  // OTHER_ENVIRONMENT_SCRIPTING_DOTNET_BINDINGS_SCENE_BINDINGS_HPP
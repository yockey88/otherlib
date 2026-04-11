/**
 * \file scripting/dotnet_bindings/component_bindings.cpp
 **/
#include "scripting/dotnet_bindings/component_bindings.hpp"

#include "object/transform.hpp"
#include "scene/scene.hpp"

#include "scripting/dotnet_bindings/scene_bindings.hpp"

namespace other {
  namespace bindings {

    void native_transform_get_position(natural_t id, float* out_x, float* out_y, float* out_z) {
      scene* active_scene = detail::get_active_scene_checked();
      if (active_scene == nullptr) {
        return;
      }
      const transform& t = active_scene->get_transform(id);
      *out_x = t.local_position.x;
      *out_y = t.local_position.y;
      *out_z = t.local_position.z;
    }

    void native_transform_set_position(natural_t id, float x, float y, float z) {
      scene* active_scene = detail::get_active_scene_checked();
      if (active_scene == nullptr) {
        return;
      }
      transform& t = active_scene->get_transform(id);
      t.local_position = glm::vec3(x, y, z);
    }

    void native_transform_get_rotation(natural_t id, float* out_x, float* out_y, float* out_z, float* out_w) {
      scene* active_scene = detail::get_active_scene_checked();
      if (active_scene == nullptr) {
        return;
      }
      const transform& t = active_scene->get_transform(id);
      *out_x = t.local_rotation_quat.x;
      *out_y = t.local_rotation_quat.y;
      *out_z = t.local_rotation_quat.z;
      *out_w = t.local_rotation_quat.w;
    }

    void native_transform_set_rotation(natural_t id, float x, float y, float z, float w) {
      scene* active_scene = detail::get_active_scene_checked();
      if (active_scene == nullptr) {
        return;
      }
      transform t = active_scene->get_transform(id);
      t.local_rotation_quat = glm::quat(w, x, y, z);
      active_scene->set_transform(id, t);
    }

    void native_transform_get_scale(natural_t id, float* out_x, float* out_y, float* out_z) {
      scene* active_scene = detail::get_active_scene_checked();
      if (active_scene == nullptr) {
        return;
      }
      const transform& t = active_scene->get_transform(id);
      *out_x = t.local_scale.x;
      *out_y = t.local_scale.y;
      *out_z = t.local_scale.z;
    }

    void native_transform_set_scale(natural_t id, float x, float y, float z) {
      scene* active_scene = detail::get_active_scene_checked();
      if (active_scene == nullptr) {
        return;
      }
      transform t = active_scene->get_transform(id);
      t.local_scale = glm::vec3(x, y, z);
      active_scene->set_transform(id, t);
    }

    void native_transform_get_world_matrix(natural_t id, float* out_matrix) {
      scene* active_scene = detail::get_active_scene_checked();
      if (active_scene == nullptr) {
        return;
      }
      glm::mat4 world = active_scene->get_world_transform(id);
      std::memcpy(out_matrix, &world[0][0], sizeof(float) * 16);
    }

  }  // namespace bindings
}  // namespace other
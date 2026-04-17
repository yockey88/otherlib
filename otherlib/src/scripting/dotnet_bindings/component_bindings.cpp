/**
 * \file scripting/dotnet_bindings/component_bindings.cpp
 **/
#include "scripting/dotnet_bindings/component_bindings.hpp"

#include "object/transform.hpp"
#include "scene/scene.hpp"

#include "scripting/dotnet_bindings/scene_bindings.hpp"

namespace other {
  namespace bindings {

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
/**
 * \file scene/scene_storage.cpp
 **/
#include "scene/scene_storage.hpp"

#include "core/defines.hpp"

#include "physics/physics_environment.hpp"
#include "script/scripting_environment.hpp"

#include "object/animation_controller.hpp"
#include "object/camera_component.hpp"
#include "object/light_component.hpp"
#include "object/physics_component.hpp"
#include "object/render_component.hpp"
#include "object/script_component.hpp"
#include "object/transform.hpp"
#include "scene/scene.hpp"

namespace other {

  scope<scene_storage> make_scene_storage(scene* scene_ptr) {
    auto* env = subsystem<scripting_environment>::get();
    OTHER_ASSERT(env != nullptr, "Failed to retrieve scripting environment");

    auto* type_db = subsystem<type_database>::get();
    OTHER_ASSERT(type_db != nullptr, "Type database subsystem is not initialized.");

    scope<scene_storage> storage = make_scope<scene_storage>(env->get_lua_host().get_lua_state());
    OTHER_ASSERT(storage != nullptr, "Failed to allocate scene storage->");

    storage->scene_id = scene_ptr->id;
    storage->tree = scene_tree{ scene_ptr };
    storage->registry = entt::registry{};
    storage->render_data_cache = std::nullopt;

    auto* physics_env = subsystem<physics_environment>::get();
    OTHER_ASSERT(physics_env != nullptr, "Physics environment subsystem is not initialized.");

    storage->physics = physics_env->create_world(scene_ptr->id);

    return storage;
  }

  void clear_storage(scope<scene_storage>& storage) {
    auto* env = subsystem<physics_environment>::get();
    OTHER_ASSERT(env != nullptr, "Physics environment subsystem is not initialized.");

    env->destroy_world(storage->scene_id);

    storage = nullptr;
  }

}  // namespace other
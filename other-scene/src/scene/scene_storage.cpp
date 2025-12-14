/**
 * \file scene/scene_storage.cpp
 **/
#include "scene/scene_storage.hpp"

#include "script/scripting_environment.hpp"

#include "scene/scene.hpp"

namespace other {

  scope<scene_storage> make_scene_storage(scene* scene_ptr) {
    auto* env = subsystem<scripting_environment>::get();
    OTHER_ASSERT(env != nullptr, "Failed to retrieve scripting environment");

    scope<scene_storage> storage = make_scope<scene_storage>(env->get_lua_host().get_lua_state());
    OTHER_ASSERT(storage != nullptr, "Failed to allocate scene storage->");

    storage->scene_id = scene_ptr->id;
    storage->tree = scene_tree{ scene_ptr };
    storage->registry = entt::registry{};
    storage->render_data_cache = std::nullopt;

    storage->sandbox["__native_scene"] = env->get_lua_host().get_lua_state().create_table();

    return storage;
  }

}  // namespace other
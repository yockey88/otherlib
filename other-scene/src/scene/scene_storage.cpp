/**
 * \file scene/scene_storage.cpp
 **/
#include "scene/scene_storage.hpp"

#include "scene/scene.hpp"

namespace other {

  scope<scene_storage> make_scene_storage(scene* scene_ptr) {
    scope<scene_storage> storage = make_scope<scene_storage>();
    OTHER_ASSERT(storage != nullptr, "Failed to allocate scene storage->");

    storage->scene_id = scene_ptr->id;
    storage->tree = scene_tree{ scene_ptr };
    storage->registry = entt::registry{};
    storage->render_data_cache = std::nullopt;

    return storage;
  }

}  // namespace other
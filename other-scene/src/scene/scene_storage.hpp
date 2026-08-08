/**
 * \file scene/scene_storage.hpp
 **/
#ifndef OTHER_SCENE_SCENE_SCENE_STORAGE_HPP
#define OTHER_SCENE_SCENE_SCENE_STORAGE_HPP

#include <entt/entt.hpp>
#include <sol/forward.hpp>

#include "core/defines.hpp"
#include "core/scope.hpp"

#include "physics_world/physics_world.hpp"
#include "renderer/renderer.hpp"

#include "scene/scene_network_context.hpp"
#include "scene/scene_tree.hpp"

namespace other {

  class scene;
  class lua_sandbox;

  struct scene_storage {
    natural_t scene_id = 0;

    entt::registry registry = {};
    entt::entity scene_root_entity = entt::null;

    scene_tree tree;

    sol::state& lua_state;
    /// behind scope<> so sol2 stays out of this header; never null after construction
    scope<lua_sandbox> sandbox;

    physics_world* physics = nullptr;

    /// net-identity registry + replication role; session-scoped runtime state
    scene_network_context network;

    glm::vec4 clear_color = glm::vec4(0.2f, 0.22f, 0.233f, 1.0f);
    std::optional<render_data> render_data_cache = std::nullopt;

    scene_storage(sol::state& lua_env);
    ~scene_storage();
  };

  scope<scene_storage> make_scene_storage(scene* scene_ptr);
  void clear_storage(scope<scene_storage>& storage_ptr);

}  // namespace other

#endif  // OTHER_SCENE_SCENE_SCENE_STORAGE_HPP
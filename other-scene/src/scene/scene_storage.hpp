/**
 * \file scene/scene_storage.hpp
 **/
#ifndef OTHER_SCENE_SCENE_SCENE_STORAGE_HPP
#define OTHER_SCENE_SCENE_SCENE_STORAGE_HPP

#include <entt/entt.hpp>

#include "core/defines.hpp"
#include "core/scope.hpp"

#include "lua/lua_sandbox.hpp"
#include "renderer/renderer.hpp"

#include "scene/scene_tree.hpp"

namespace other {

  class scene;

  struct scene_storage {
    natural_t scene_id = 0;
    entt::registry registry = {};
    scene_tree tree;

    sol::state& lua_state;
    lua_sandbox sandbox;

    std::optional<render_data> render_data_cache = std::nullopt;

    scene_storage(sol::state& lua_env)
        : lua_state(lua_env), sandbox(lua_env) {}
  };

  scope<scene_storage> make_scene_storage(scene* scene_ptr);

}  // namespace other

#endif  // OTHER_SCENE_SCENE_SCENE_STORAGE_HPP
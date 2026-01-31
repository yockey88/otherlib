/**
 * \file scene/scene_storage.hpp
 **/
#ifndef OTHER_SCENE_SCENE_SCENE_STORAGE_HPP
#define OTHER_SCENE_SCENE_SCENE_STORAGE_HPP

#include <entt/entt.hpp>

#include "core/defines.hpp"
#include "core/scope.hpp"

#include "lua/lua_sandbox.hpp"
#include "physics_world/physics_world.hpp"
#include "renderer/renderer.hpp"

#include "scene/scene_tree.hpp"

namespace other {

  class scene;

  struct component_reflection_data {
    using custom_registration_fn = void (*)(lua_sandbox& sandbox);
    using custom_has_component_registration_fn = void (*)(bool, lua_sandbox& sandbox);
    using add_component_fn = void (*)(entt::registry&, uint32_t, lua_sandbox&, component_reflection_data::custom_registration_fn);
    using remove_component_fn = void (*)(entt::registry&, uint32_t, lua_sandbox&, component_reflection_data::custom_registration_fn);
    using has_component_fn = bool (*)(entt::registry&, uint32_t, lua_sandbox&, component_reflection_data::custom_has_component_registration_fn);

    struct registration_fn {
      custom_registration_fn add_fn = nullptr;
      custom_registration_fn remove_fn = nullptr;
      custom_has_component_registration_fn has_fn = nullptr;
    };

    struct type_data {
      reflection_data* reflection = nullptr;
      add_component_fn add_fn = nullptr;
      remove_component_fn remove_fn = nullptr;
      has_component_fn has_fn = nullptr;
      registration_fn custom_registration;
    };

    std::map<natural_t, type_data> component_types;
  };

  struct scene_storage {
    natural_t scene_id = 0;

    entt::registry registry = {};
    entt::entity scene_root_entity = entt::null;

    scene_tree tree;

    sol::state& lua_state;
    lua_sandbox sandbox;

    physics_world* physics = nullptr;

    glm::vec4 clear_color = glm::vec4(0.2f, 0.22f, 0.233f, 1.0f);
    std::optional<render_data> render_data_cache = std::nullopt;

    component_reflection_data reflection_data;

    scene_storage(sol::state& lua_env)
        : lua_state(lua_env), sandbox(lua_env) {}
  };

  scope<scene_storage> make_scene_storage(scene* scene_ptr);
  void clear_storage(scope<scene_storage>& storage_ptr);

}  // namespace other

#endif  // OTHER_SCENE_SCENE_SCENE_STORAGE_HPP
/**
 * \file scene/scene_storage.cpp
 **/
#include "scene/scene_storage.hpp"

#include "core/defines.hpp"

#include "physics/physics_environment.hpp"
#include "script/scripting_environment.hpp"

#include "object/camera_component.hpp"
#include "object/render_component.hpp"
#include "object/script_component.hpp"
#include "object/transform.hpp"
#include "scene/scene.hpp"

namespace other {

  template <typename CT>
  void bind_component_reflection_data(type_database& type_db, scope<scene_storage>& storage_ptr, component_reflection_data::registration_fn lua_reg_fn = {}) {
    auto* refl_data = type_db.get_reflection_data<CT>(CT{});
    OTHER_ASSERT(refl_data != nullptr, "Reflection data for component type '{}' is null.", typeid(CT).name());

    component_reflection_data::type_data type_info;
    type_info.reflection = refl_data;
    type_info.custom_registration = lua_reg_fn;
    type_info.add_fn = [](entt::registry& reg, uint32_t ent, lua_sandbox& sandbox, component_reflection_data::custom_registration_fn custom_fn = nullptr) {
      reg.emplace<CT>(entt::entity(ent));
      if (custom_fn != nullptr) {
        custom_fn(sandbox);
      }
    };
    type_info.remove_fn = [](entt::registry& reg, uint32_t ent, lua_sandbox& sandbox, component_reflection_data::custom_registration_fn custom_fn = nullptr) {
      reg.remove<CT>(entt::entity(ent));
      if (custom_fn != nullptr) {
        custom_fn(sandbox);
      }
    };
    type_info.has_fn = [](entt::registry& reg, uint32_t ent, lua_sandbox& sandbox, component_reflection_data::custom_has_component_registration_fn custom_fn = nullptr) -> bool {
      bool has_comp = reg.try_get<CT>(entt::entity(ent)) != nullptr;
      if (custom_fn != nullptr) {
        custom_fn(has_comp, sandbox);
      }
      return has_comp;
    };

    auto [itr, inserted] = storage_ptr->reflection_data.component_types.emplace(refl_data->type_hash, type_info);
    OTHER_ASSERT(inserted, "Component type '{}' already exists in reflection storage.", refl_data->type_name);
  }

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

    bind_component_reflection_data<transform>(*type_db, storage);
    bind_component_reflection_data<script_component>(*type_db, storage);
    bind_component_reflection_data<render_component>(*type_db, storage);
    bind_component_reflection_data<camera_component>(*type_db, storage);

    auto* physics_env = subsystem<physics_environment>::get();
    OTHER_ASSERT(physics_env != nullptr, "Physics environment subsystem is not initialized.");

    storage->physics = physics_env->create_world(scene_ptr->id);

    return storage;
  }

  void clear_storage(scope<scene_storage>& storage) {
    storage->tree.destroy_all_objects();

    auto* env = subsystem<physics_environment>::get();
    OTHER_ASSERT(env != nullptr, "Physics environment subsystem is not initialized.");

    env->destroy_world(storage->scene_id);

    storage = nullptr;
  }

}  // namespace other
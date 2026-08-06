/**
 * \file physics/physics_environment.cpp
 **/
#include "physics/physics_environment.hpp"

#include <toml++/toml.h>

#include "core/fnv.hpp"

#include "physics/backends/box3d_api.hpp"
#include "physics/backends/jolt_api.hpp"

namespace other {

  namespace backend_keys {

    static constexpr std::string_view kJolt = "jolt";
    static constexpr natural_t kJoltHash = FNV(kJolt);

    static constexpr std::string_view kBox3d = "box3d";
    static constexpr natural_t kBox3dHash = FNV(kBox3d);

  }  // namespace backend_keys

  void physics_environment::load_backend(const config_table& config) {
    PROFILE_SECTION("physics_environment::load_backend");

    natural_t backend_hash = FNV(config.get_value<std::string>("physics.backend", "jolt"));
    switch (backend_hash) {
      case backend_keys::kJoltHash: set_physics_api(make_scope<jolt_api>(), config); break;
      case backend_keys::kBox3dHash: set_physics_api(make_scope<box3d_api>(), config); break;
      default:
        OTHER_ASSERT(false, "Unknown/Unimplemented physics backend: {}", config.get_value<std::string>("physics.backend", "jolt"));
        break;
    }
  }

  void physics_environment::unload_backend() {
    physics_backend->shutdown();
    physics_backend = nullptr;
  }

  void physics_environment::initialize_physics_environment(const config_table& configuration) {
    physics_bodies = make_scope<memory_pool<physics_body>>();
    physics_shapes = make_scope<memory_pool<physics_shape>>();
    fixed_step = configuration.get_value<float>("physics.fixed_step", kDefaultFixedStep);

    /// per-world creation settings; backends consume what applies to them
    if (configuration.has_path("physics.gravity")) {
      const toml::array* arr = configuration.get_raw("physics.gravity").as_array();
      if (arr != nullptr && arr->size() == 3) {
        default_world_config.gravity = {
          static_cast<float>(arr->get(0)->value_or(0.0)),
          static_cast<float>(arr->get(1)->value_or(0.0)),
          static_cast<float>(arr->get(2)->value_or(0.0)),
        };
      } else {
        CORE_LOG_WARN("physics.gravity must be a 3-element array, using default ({}, {}, {}).",
                      default_world_config.gravity.x, default_world_config.gravity.y, default_world_config.gravity.z);
      }
    }
    default_world_config.max_bodies = configuration.get_value<uint32_t>("physics.max-bodies", default_world_config.max_bodies);
    default_world_config.max_body_pairs = configuration.get_value<uint32_t>("physics.max-body-pairs", default_world_config.max_body_pairs);
    default_world_config.max_contact_constraints = configuration.get_value<uint32_t>("physics.max-contact-constraints", default_world_config.max_contact_constraints);
  }

  void physics_environment::shutdown_physics_environment() {
    /// drain through destroy_world so every backend world shuts down, not just the map entries
    while (!worlds.empty()) {
      destroy_world(worlds.begin()->first);
    }
    physics_shapes = nullptr;
    physics_bodies = nullptr;
  }

  physics_world* physics_environment::create_world(natural_t id) {
    {
      auto itr = worlds.find(id);
      if (itr != worlds.end()) {
        CORE_LOG_WARN("Physics world with id '{}' already exists.", id);
        return itr->second;
      }
    }

    physics_world* world = arena_allocator<physics_world>{}.allocate(physics_bodies, physics_shapes);
    OTHER_ASSERT(world != nullptr, "Failed to allocate physics world '{}'.", id);

    auto [itr, inserted] = worlds.emplace(id, world);
    OTHER_ASSERT(inserted, "Failed to insert physics world '{}' into worlds map.", id);

    itr->second->initialize(id, default_world_config);
    return world;
  }

  void physics_environment::destroy_world(natural_t id) {
    auto itr = worlds.find(id);
    if (itr == worlds.end()) {
      CORE_LOG_ERROR("Physics world with id '{}' does not exist.", id);
      return;
    }

    itr->second->shutdown();
    arena_allocator<physics_world>{}.free(itr->second);
    worlds.erase(itr);
  }

  void physics_environment::set_physics_api(scope<physics_api> api, const config_table& config) {
    OTHER_ASSERT(api != nullptr, "Physics API instance cannot be null.");
    PROFILE_SECTION("physics_environment::set-physics-api");
    physics_backend = std::move(api);
    physics_backend->initialize(config);
  }

}  // namespace other

/**
 * \file physics/physics_environment.cpp
 **/
#include "physics/physics_environment.hpp"

#include "core/fnv.hpp"

#include "physics/backends/jolt_api.hpp"
// #include "physics/backends/physx_api.hpp"

namespace other {

  namespace backend_keys {

    // static constexpr std::string_view kPhysX = "physx";
    static constexpr std::string_view kJolt = "jolt";

    // static constexpr natural_t kPhysXHash = FNV(kPhysX);
    static constexpr natural_t kJoltHash = FNV(kJolt);

  }  // namespace backend_keys

  void physics_environment::load_backend(const config_table& config) {
    PROFILE_SECTION("physics_environment::load_backend");

    natural_t backend_hash = FNV(config.get_value<std::string>("physics.backend", "jolt"));
    switch (backend_hash) {
      case backend_keys::kJoltHash: set_phsyics_api(make_scope<jolt_api>(), config); break;
      // case backend_keys::kPhysXHash: set_phsyics_api(make_scope<physx_api>(), config); break;
      default:
        OTHER_ASSERT(false, "Unknown/Unimplmented physics backend: {}", config.get_value<std::string>("physics.backend", "jolt"));
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
  }

  void physics_environment::shutdown_physics_environment() {
    worlds.clear();
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

    itr->second->initialize(id);
    return world;
  }

  void physics_environment::destroy_world(natural_t id) {
    auto itr = worlds.find(id);
    if (itr == worlds.end()) {
      CORE_LOG_ERROR("Physics world with id '{}' does not exist.", id);
      return;
    }

    arena_allocator<physics_world>{}.free(itr->second);
    worlds.erase(itr);
  }

  void physics_environment::set_phsyics_api(scope<physics_api> api, const config_table& config) {
    OTHER_ASSERT(api != nullptr, "Rendering API instance cannot be null.");
    PROFILE_SECTION("physics_environment::set-physics-api");
    physics_backend = std::move(api);
    physics_backend->initialize(config);
  }

}  // namespace other
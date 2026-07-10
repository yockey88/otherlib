/**
 * \file physics/physics_environment.hpp
 **/
#ifndef OTHER_PHYSICS_PHYSICS_PHYSICS_ENVIRONMENT_HPP
#define OTHER_PHYSICS_PHYSICS_PHYSICS_ENVIRONMENT_HPP

#include "core/config_table.hpp"
#include "core/scope.hpp"
#include "core/subsystem.hpp"

#include "physics/physics_api.hpp"
#include "physics_world/physics_world.hpp"

namespace other {

  class physics_environment : public subsystem<physics_environment> {
   public:
    physics_environment() = default;
    virtual ~physics_environment() = default;

    scope<physics_api>& api() { return physics_backend; }
    bool has_api() const { return physics_backend != nullptr; }

    void load_backend(const config_table& config);
    void unload_backend();

    void initialize_physics_environment(const config_table& configuration);
    void shutdown_physics_environment();

    physics_world* create_world(natural_t id);
    void destroy_world(natural_t id);

   private:
    ostd::map<natural_t, physics_world*> worlds;
    scope<physics_api> physics_backend = nullptr;

    scope<memory_pool<physics_body>> physics_bodies = nullptr;
    scope<memory_pool<physics_shape>> physics_shapes = nullptr;

    void set_phsyics_api(scope<physics_api> api, const config_table& config);
  };

}  // namespace other

OTHER_DEPENDENT_SUBSYSTEM(
  other::physics_environment,
  subsystem_profile::kArena,
  subsystem_profile::kLogger);

#endif  // OTHER_PHYSICS_PHYSICS_PHYSICS_ENVIRONMENT_HPP
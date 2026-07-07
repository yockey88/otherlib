/**
 * \file physics/backends/physx_api.hpp
 **/
#ifndef OTHER_PHYSICS_PHYSICS_BACKENDS_PHYSX_API_HPP
#define OTHER_PHYSICS_PHYSICS_BACKENDS_PHYSX_API_HPP

#include "physics/physics_api.hpp"

namespace physx {
  class PxFoundation;
  class PxPhysics;
  class PxPvd;
  class PxOmniPvd;

  class PxScene;
  class PxRigidStatic;
  class PxRigidDynamic;
}  // namespace physx

namespace other {

  class physx_api : public physics_api {
   public:
    physx_api() = default;
    virtual ~physx_api() = default;

    physics_render_debug_data get_debug_render_data(natural_t id, const physics_world* world) const override;

    void on_initialize(const config_table& configuration) override;
    void on_shutdown() override;

    void initialize_world(natural_t id, physics_world* world) override;
    void shutdown_world(physics_world* world) override;

    void register_physics_body(natural_t world_id, physics_world* world, physics_body* body) override;
    void unregister_physics_body(natural_t world_id, physics_world* world, physics_body* body) override;

    void attach_shape(natural_t world_id, physics_world* world, physics_body* body, physics_shape* shape) override;
    void detach_shape(natural_t world_id, physics_world* world, physics_body* body, physics_shape* shape) override;

    void configure_empty_shape(physics_shape* shape) override;
    void configure_box_shape(physics_shape* shape, const glm::vec3& half_extents) override;

    void step_simulation(natural_t world_id, physics_world* world, double delta_time) override;
    void update_active_transforms(natural_t world_id, physics_world* world, double delta_time) override;

   private:
    physx::PxFoundation* foundation = nullptr;
    physx::PxPhysics* physics = nullptr;
    physx::PxPvd* pvd = nullptr;
    physx::PxOmniPvd* omni_pvd = nullptr;

    ostd::map<natural_t, physx::PxScene*> physx_scenes;
  };

}  // namespace other

#endif  // OTHER_PHYSICS_PHYSICS_BACKENDS_PHYSX_API_HPP
/**
 * \file physics/backends/jolt_api.hpp
 **/
#ifndef OTHER_PHYSICS_PHYSICS_BACKENDS_JOLT_API_HPP
#define OTHER_PHYSICS_PHYSICS_BACKENDS_JOLT_API_HPP

#include "physics/physics_api.hpp"

namespace JPH {
  class PhysicsSystem;
  class TempAllocator;
  class JobSystemThreadPool;
}  // namespace JPH

namespace other {

  class BPLayerInterfaceImpl;
  class ObjectVsBroadPhaseLayerFilterImpl;
  class ObjectLayerPairFilterImpl;

  class jolt_api : public physics_api {
   public:
    jolt_api() = default;
    virtual ~jolt_api() = default;

    physics_render_debug_data get_debug_render_data(natural_t id, const physics_world* world) const override;

    void initialize_world(natural_t id, physics_world* world) override;
    void shutdown_world(physics_world* world) override;

    void on_scene_start(natural_t world_id, physics_world* world) override;
    // void on_scene_stop(natural_t world_id, physics_world* world) override;

    void register_physics_body(natural_t world_id, physics_world* world, physics_body* body) override;
    void unregister_physics_body(natural_t world_id, physics_world* world, physics_body* body) override;

    void attach_shape(natural_t world_id, physics_world* world, physics_body* body, physics_shape* shape) override;
    void detach_shape(natural_t world_id, physics_world* world, physics_body* body, physics_shape* shape) override;

    void configure_empty_shape(physics_shape* shape) override;
    void configure_box_shape(physics_shape* shape, const glm::vec3& half_extents) override;

    void step_simulation(natural_t world_id, physics_world* world, double delta_time) override;
    void update_active_transforms(natural_t world_id, physics_world* world, double delta_time) override;

   private:
    JPH::TempAllocator* temp_allocator = nullptr;
    JPH::JobSystemThreadPool* job_system = nullptr;

    BPLayerInterfaceImpl* bp_layer_interface = nullptr;
    ObjectVsBroadPhaseLayerFilterImpl* object_vs_broadphase_layer_filter = nullptr;
    ObjectLayerPairFilterImpl* object_layer_pair_filter = nullptr;

    std::map<natural_t, JPH::PhysicsSystem*> jolt_scenes;

    std::map<natural_t, uint32_t> jolt_body_ids;
    std::map<uint32_t, void*> jolt_shapes;

    void on_initialize(const config_table& configuration) override;
    void on_shutdown() override;
  };

}  // namespace other

#endif  // OTHER_PHYSICS_PHYSICS_BACKENDS_JOLT_API_HPP
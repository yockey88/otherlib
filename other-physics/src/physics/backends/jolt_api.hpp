/**
 * \file physics/backends/jolt_api.hpp
 **/
#ifndef OTHER_PHYSICS_PHYSICS_BACKENDS_JOLT_API_HPP
#define OTHER_PHYSICS_PHYSICS_BACKENDS_JOLT_API_HPP

#include "physics/physics_api.hpp"

namespace JPH {
  class TempAllocator;
  class JobSystemThreadPool;
}  // namespace JPH

namespace other {

  class BPLayerInterfaceImpl;
  class ObjectVsBroadPhaseLayerFilterImpl;
  class ObjectLayerPairFilterImpl;

  /// per-world backend state (system + listener + this world's body map); defined in the .cpp
  struct jolt_world;

  class jolt_api : public physics_api {
   public:
    jolt_api() = default;
    virtual ~jolt_api() = default;

    physics_render_debug_data get_debug_render_data(natural_t id, const physics_world* world) const override;

    void initialize_world(natural_t id, physics_world* world, const physics_world_config& config) override;
    void shutdown_world(physics_world* world) override;

    void on_scene_start(natural_t world_id, physics_world* world) override;
    // void on_scene_stop(natural_t world_id, physics_world* world) override;

    void register_physics_body(natural_t world_id, physics_world* world, physics_body* body) override;
    void unregister_physics_body(natural_t world_id, physics_world* world, physics_body* body) override;

    void teleport_body(natural_t world_id, physics_world* world, physics_body* body, const glm::mat4& world_transform) override;
    void move_kinematic(natural_t world_id, physics_world* world, physics_body* body, const glm::mat4& world_transform, double step) override;

    bool set_body_shape(natural_t world_id, physics_world* world, physics_body* body,
                        const physics_shape_desc& desc, const glm::vec3& world_scale,
                        const shape_geometry* geometry) override;

    void step_simulation(natural_t world_id, physics_world* world, double delta_time) override;
    void update_active_transforms(natural_t world_id, physics_world* world, double delta_time) override;

    void drain_contacts(natural_t world_id, physics_world* world, ostd::vector<contact_event>& out) override;

    raycast_hit cast_ray(natural_t world_id, physics_world* world, const glm::vec3& origin,
                         const glm::vec3& direction, float max_distance) override;

    integer_t create_fixed_joint(natural_t world_id, physics_world* world, physics_body* body_a, physics_body* body_b) override;
    void destroy_joint(natural_t world_id, physics_world* world, integer_t joint_id) override;
    float joint_reaction_force(natural_t world_id, physics_world* world, integer_t joint_id, double step) override;

    void set_linear_velocity(natural_t world_id, physics_world* world, physics_body* body, const glm::vec3& velocity) override;
    glm::vec3 get_linear_velocity(natural_t world_id, physics_world* world, physics_body* body) override;
    void set_angular_velocity(natural_t world_id, physics_world* world, physics_body* body, const glm::vec3& velocity) override;
    glm::vec3 get_angular_velocity(natural_t world_id, physics_world* world, physics_body* body) override;
    void add_force(natural_t world_id, physics_world* world, physics_body* body, const glm::vec3& force) override;
    void add_impulse(natural_t world_id, physics_world* world, physics_body* body, const glm::vec3& impulse) override;
    void add_torque(natural_t world_id, physics_world* world, physics_body* body, const glm::vec3& torque) override;

   private:
    JPH::TempAllocator* temp_allocator = nullptr;
    JPH::JobSystemThreadPool* job_system = nullptr;

    BPLayerInterfaceImpl* bp_layer_interface = nullptr;
    ObjectVsBroadPhaseLayerFilterImpl* object_vs_broadphase_layer_filter = nullptr;
    ObjectLayerPairFilterImpl* object_layer_pair_filter = nullptr;

    ostd::map<natural_t, jolt_world*> jolt_worlds;

    /// main-thread scratch for per-step contact drains (std:: — carries events queued by jolt workers)
    std::vector<contact_event> scratch_events;

    jolt_world& world_state(natural_t world_id);
    const jolt_world& world_state(natural_t world_id) const;

    void on_initialize(const config_table& configuration) override;
    void on_shutdown() override;
  };

}  // namespace other

#endif  // OTHER_PHYSICS_PHYSICS_BACKENDS_JOLT_API_HPP

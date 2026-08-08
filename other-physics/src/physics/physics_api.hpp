/**
 * \file physics/physics_api.hpp
 **/
#ifndef OTHER_PHYSICS_PHYSICS_PHYSICS_API_HPP
#define OTHER_PHYSICS_PHYSICS_PHYSICS_API_HPP

#include "core/config_table.hpp"

namespace other {

  struct physics_body;
  struct physics_shape;
  struct physics_shape_desc;
  class physics_world;

  /// borrowed geometry spans for hull/mesh shape builds; extracted scene-side from the
  ///   entity's render model — physics never learns about models
  struct shape_geometry {
    std::span<const glm::vec3> positions;
    std::span<const uint32_t> indices;  /// triangle list; may be empty for hull-from-points
  };

  /// one contact transition, queued by the backend's listener (worker threads) and drained
  ///   on the main thread after each step. kEnd carries no point/normal (none exists)
  struct contact_event {
    enum kind : uint32_t {
      kBegin = 0,
      kEnd,
      kTriggerBegin,
      kTriggerEnd,
    };
    kind type = kBegin;
    integer_t body_a = -1;  /// physics_body ids; kEnd events leave the backend carrying backend
    integer_t body_b = -1;  ///   ids and the backend's drain resolves them before returning
    glm::vec3 point = {};   /// first manifold point, world space (begin events only)
    glm::vec3 normal = {};  /// a -> b contact normal (begin events only)
  };

  struct raycast_hit {
    bool hit = false;
    integer_t body_id = -1;
    natural_t owner_object_id = 0;
    glm::vec3 point = {};
    glm::vec3 normal = {};
    float distance = 0.f;
  };

  /// backend-agnostic per-world creation settings, sourced from [physics] config
  /// backends consume what applies to them and ignore the rest
  struct physics_world_config {
    glm::vec3 gravity = { 0.f, -9.81f, 0.f };
    uint32_t max_bodies = 65536;
    uint32_t max_body_pairs = 65536;
    uint32_t max_contact_constraints = 10240;
  };

  class physics_api {
   public:
    struct line {
      glm::vec3 start;
      glm::vec3 end;
    };
    struct triangle {
      glm::vec3 v0;
      glm::vec3 v1;
      glm::vec3 v2;
    };
    struct physics_render_debug_data {
      ostd::vector<line> debug_lines;
      ostd::vector<glm::vec4> debug_line_colors;
      ostd::vector<triangle> debug_triangles;
      ostd::vector<glm::vec4> debug_triangle_colors;
    };

    physics_api() = default;
    virtual ~physics_api() = default;

    void initialize(const config_table& configuration);
    void shutdown();

    virtual physics_render_debug_data get_debug_render_data(natural_t id, const physics_world* world) const = 0;

    virtual void initialize_world(natural_t id, physics_world* world, const physics_world_config& config) = 0;
    virtual void shutdown_world(physics_world* world) = 0;

    virtual void on_scene_start(natural_t world_id, physics_world* world) {}
    virtual void on_scene_stop(natural_t world_id, physics_world* world) {}

    virtual void register_physics_body(natural_t world_id, physics_world* world, physics_body* body) = 0;
    virtual void unregister_physics_body(natural_t world_id, physics_world* world, physics_body* body) = 0;

    /// hard-set a body's pose and zero its velocities, without waking it
    /// (edit-mode moves, play-time reseeding, restores)
    virtual void teleport_body(natural_t world_id, physics_world* world, physics_body* body, const glm::mat4& world_transform) = 0;

    /// sweep a kinematic body toward the target pose over one fixed step, with contact response
    virtual void move_kinematic(natural_t world_id, physics_world* world, physics_body* body, const glm::mat4& world_transform, double step) = 0;

    /// builds the described shape (world scale baked in) and attaches it to the body; geometry
    ///   required for hull/mesh. false = build failed, body keeps its previous shape (never asserts)
    virtual bool set_body_shape(natural_t world_id, physics_world* world, physics_body* body,
                                const physics_shape_desc& desc, const glm::vec3& world_scale,
                                const shape_geometry* geometry) = 0;

    virtual void step_simulation(natural_t world_id, physics_world* world, double delta_time) = 0;
    virtual void update_active_transforms(natural_t world_id, physics_world* world, double delta_time) = 0;

    /// pull the contact transitions recorded during the last step; engine body ids on return
    virtual void drain_contacts(natural_t world_id, physics_world* world, ostd::vector<contact_event>& out) = 0;

    /// closest-hit ray query; sensors are not surfaces and never hit
    virtual raycast_hit cast_ray(natural_t world_id, physics_world* world, const glm::vec3& origin,
                                 const glm::vec3& direction, float max_distance) = 0;

    /// weld two bodies rigidly; returns a backend joint id, -1 on failure
    virtual integer_t create_fixed_joint(natural_t world_id, physics_world* world, physics_body* body_a, physics_body* body_b) = 0;
    virtual void destroy_joint(natural_t world_id, physics_world* world, integer_t joint_id) = 0;
    /// force the weld sustained through the last step, newtons
    virtual float joint_reaction_force(natural_t world_id, physics_world* world, integer_t joint_id, double step) = 0;

    virtual void set_linear_velocity(natural_t world_id, physics_world* world, physics_body* body, const glm::vec3& velocity) = 0;
    virtual glm::vec3 get_linear_velocity(natural_t world_id, physics_world* world, physics_body* body) = 0;
    virtual void set_angular_velocity(natural_t world_id, physics_world* world, physics_body* body, const glm::vec3& velocity) = 0;
    virtual glm::vec3 get_angular_velocity(natural_t world_id, physics_world* world, physics_body* body) = 0;
    virtual void add_force(natural_t world_id, physics_world* world, physics_body* body, const glm::vec3& force) = 0;
    virtual void add_impulse(natural_t world_id, physics_world* world, physics_body* body, const glm::vec3& impulse) = 0;
    virtual void add_torque(natural_t world_id, physics_world* world, physics_body* body, const glm::vec3& torque) = 0;

   protected:
    virtual void on_initialize(const config_table& configuration) = 0;
    virtual void on_shutdown() = 0;
  };

}  // namespace other

#endif  // OTHER_PHYSICS_PHYSICS_PHYSICS_API_HPP

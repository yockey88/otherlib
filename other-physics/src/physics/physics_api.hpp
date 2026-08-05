/**
 * \file physics/physics_api.hpp
 **/
#ifndef OTHER_PHYSICS_PHYSICS_PHYSICS_API_HPP
#define OTHER_PHYSICS_PHYSICS_PHYSICS_API_HPP

#include "core/config_table.hpp"

namespace other {

  struct physics_body;
  struct physics_shape;
  class physics_world;

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

    virtual void attach_shape(natural_t world_id, physics_world* world, physics_body* body, physics_shape* shape) = 0;
    virtual void detach_shape(natural_t world_id, physics_world* world, physics_body* body, physics_shape* shape) = 0;

    virtual void configure_empty_shape(physics_shape* shape) = 0;
    virtual void configure_box_shape(physics_shape* shape, const glm::vec3& half_extents) = 0;
    virtual void configure_sphere_shape(physics_shape* shape, float radius) {}
    virtual void configure_capsule_shape(physics_shape* shape, float radius, float height) {}
    virtual void configure_convex_hull_shape(physics_shape* shape, const std::span<const glm::vec3> points) {}
    virtual void configure_triangle_mesh_shape(physics_shape* shape, const std::span<const glm::vec3> vertices, const std::span<const natural_t> indices) {}
    virtual void configure_heightfield_shape(physics_shape* shape, const std::span<const float> height_data, natural_t width, natural_t depth, float min_height, float max_height) {}

    virtual void step_simulation(natural_t world_id, physics_world* world, double delta_time) = 0;
    virtual void update_active_transforms(natural_t world_id, physics_world* world, double delta_time) = 0;

   protected:
    virtual void on_initialize(const config_table& configuration) = 0;
    virtual void on_shutdown() = 0;
  };

}  // namespace other

#endif  // OTHER_PHYSICS_PHYSICS_PHYSICS_API_HPP

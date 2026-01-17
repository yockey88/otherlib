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
      std::vector<line> debug_lines;
      std::vector<glm::vec4> debug_line_colors;

      std::vector<triangle> debug_triangles;
      std::vector<glm::vec4> debug_triangle_colors;
    };

    physics_api() = default;
    virtual ~physics_api() = default;

    void initialize(const config_table& configuration);
    void shutdown();

    inline float get_interpolation_alpha() const {
      return alpha;
    }

    virtual physics_render_debug_data get_debug_render_data(natural_t id, const physics_world* world) const = 0;

    virtual void initialize_world(natural_t id, physics_world* world) = 0;
    virtual void shutdown_world(physics_world* world) = 0;

    virtual void on_scene_start(natural_t world_id, physics_world* world) {}
    virtual void on_scene_stop(natural_t world_id, physics_world* world) {}

    virtual void register_physics_body(natural_t world_id, physics_world* world, physics_body* body) = 0;
    virtual void unregister_physics_body(natural_t world_id, physics_world* world, physics_body* body) = 0;

    virtual void attach_shape(natural_t world_id, physics_world* world, physics_body* body, physics_shape* shape) = 0;
    virtual void detach_shape(natural_t world_id, physics_world* world, physics_body* body, physics_shape* shape) = 0;

    virtual void configure_empty_shape(physics_shape* shape) = 0;
    virtual void configure_box_shape(physics_shape* shape, const glm::vec3& half_extents) = 0;
    virtual void configure_sphere_shape(physics_shape* shape, float radius) {}
    virtual void configure_capsule_shape(physics_shape* shape, float radius, float height) {}
    virtual void configure_convex_hull_shape(physics_shape* shape, const std::vector<glm::vec3>& points) {}
    virtual void configure_triangle_mesh_shape(physics_shape* shape, const std::vector<glm::vec3>& vertices, const std::vector<natural_t>& indices) {}
    virtual void configure_heightfield_shape(physics_shape* shape, const std::vector<float>& height_data, natural_t width, natural_t depth, float min_height, float max_height) {}

    virtual void step_simulation(natural_t world_id, physics_world* world, double delta_time) = 0;
    virtual void update_active_transforms(natural_t world_id, physics_world* world, double delta_time) = 0;

   protected:
    float alpha = 0.0f;
    float accumulator = 0.0f;
    constexpr static float kFixedTimeStep = 1.0f / 60.0f;

    virtual void on_initialize(const config_table& configuration) = 0;
    virtual void on_shutdown() = 0;

   private:
  };

}  // namespace other

#endif  // OTHER_PHYSICS_PHYSICS_PHYSICS_API_HPP
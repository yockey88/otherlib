/**
 * \file physics_world/physics_world.hpp
 **/
#ifndef OTHER_PHYSICS_PHYSICS_WORLD_PHYSICS_WORLD_HPP
#define OTHER_PHYSICS_PHYSICS_WORLD_PHYSICS_WORLD_HPP

#include <string>

#include "core/defines.hpp"
#include "core/memory_pool.hpp"
#include "core/scope.hpp"

#include "physics/physics_api.hpp"
#include "physics_world/physics_body.hpp"

#include "physics_shape.hpp"

namespace other {

  class physics_world {
   private:
    struct live_body {
      size_t id = 0;
      physics_body* object = nullptr;
      physics_shape* shape = nullptr;
    };

   public:
    physics_world(scope<memory_pool<physics_body>>& physics_bodies, scope<memory_pool<physics_shape>>& physics_shapes);
    ~physics_world() = default;

    physics_api::physics_render_debug_data get_debug_render_data() const;

    void initialize(natural_t id);
    void shutdown();

    void start_simulation();
    void stop_simulation();

    void step_simulation(double delta_time);

    physics_body* create_physics_body(const physics_body_settings& settings);
    void destroy_physics_body(physics_body* body);

    physics_shape* create_empty_shape(physics_body* body);
    physics_shape* create_box_shape(physics_body* body, const glm::vec3& half_extents);
    physics_shape* create_sphere_shape(physics_body* body, float radius);
    physics_shape* create_capsule_shape(physics_body* body, float radius, float height);
    physics_shape* create_convex_hull_shape(physics_body* body, const std::vector<glm::vec3>& points);
    physics_shape* create_triangle_mesh_shape(physics_body* body, const std::vector<glm::vec3>& vertices, const std::vector<natural_t>& indices);
    physics_shape* create_heightfield_shape(physics_body* body, const std::vector<float>& height_data, natural_t width, natural_t depth, float min_height, float max_height);
    void destroy_physics_shape(physics_shape* shape);

    template <typename Fn>
      requires std::is_invocable_r_v<bool, Fn, physics_body*>
    physics_body* find_if(Fn&& predicate) {
      for (auto& live_obj : live_objects) {
        if (live_obj.object != nullptr && predicate(live_obj.object)) {
          return live_obj.object;
        }
      }
      return nullptr;
    }

    std::string name = "Physics-World";
    natural_t world_id = 0;

   private:
    scope<class physics_api>& physics_api();
    const scope<class physics_api>& physics_api() const;

    scope<memory_pool<physics_body>>& physics_bodies;
    scope<memory_pool<physics_shape>>& physics_shapes;

    constexpr static inline size_t kMaxPhysicsBodies = memory_pool<physics_body>::kMaxObjects;
    std::array<live_body, kMaxPhysicsBodies> live_objects = {};

    void interpolate_active_transforms(double delta_time);
    glm::mat4 interpolate_transform(const glm::mat4& previous, const glm::mat4& current) const;
  };

}  // namespace other

#endif  // OTHER_PHYSICS_PHYSICS_WORLD_PHYSICS_WORLD_HPP
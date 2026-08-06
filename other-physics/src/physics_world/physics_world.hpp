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

    void initialize(natural_t id, const physics_world_config& config);
    void shutdown();

    void start_simulation();
    void stop_simulation();

    void step_simulation(double delta_time);

    /// hard-set a body's pose (and zero velocities) without waking it
    void teleport_body(physics_body* body, const glm::mat4& world_transform);
    /// sweep a kinematic body toward the target pose over one fixed step
    void move_kinematic(physics_body* body, const glm::mat4& world_transform, double step);
    void interpolate_active_transforms(double alpha);

    physics_body* create_physics_body(const physics_body::settings& settings, const glm::mat4& world_transform);
    void destroy_physics_body(physics_body* body);

    /// build/rebuild the body's collider from its authored desc (allocating the pool shape on
    ///   first call); geometry required for hull/mesh kinds, fit_bounds for fit_render_bounds.
    ///   mesh on a non-static body downgrades to a hull with a warning. a build that cannot
    ///   proceed yet leaves `applied` untouched so the revalidation pass retries.
    ///   returns the body's shape record
    physics_shape* apply_shape(physics_body* body, const physics_shape_desc& desc,
                               const glm::vec3& world_scale, const shape_geometry* geometry = nullptr,
                               const bounding_box* fit_bounds = nullptr);
    void destroy_physics_shape(physics_shape* shape);

    /// direct pool-index lookup; nullptr for freed/out-of-range ids
    physics_body* body_by_id(integer_t id) {
      if (id < 0 || static_cast<size_t>(id) >= kMaxPhysicsBodies) {
        return nullptr;
      }
      return live_objects[static_cast<size_t>(id)].object;
    }

    /// pull the last step's contact transitions; sensor end events are re-flagged here
    void drain_contacts(ostd::vector<contact_event>& out);

    raycast_hit cast_ray(const glm::vec3& origin, const glm::vec3& direction, float max_distance);

    integer_t create_fixed_joint(physics_body* body_a, physics_body* body_b);
    void destroy_joint(integer_t joint_id);
    float joint_reaction_force(integer_t joint_id, double step);

    void set_linear_velocity(physics_body* body, const glm::vec3& velocity);
    glm::vec3 get_linear_velocity(physics_body* body);
    void set_angular_velocity(physics_body* body, const glm::vec3& velocity);
    glm::vec3 get_angular_velocity(physics_body* body);
    void add_force(physics_body* body, const glm::vec3& force);
    void add_impulse(physics_body* body, const glm::vec3& impulse);
    void add_torque(physics_body* body, const glm::vec3& torque);

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

    glm::mat4 interpolate_transform(const glm::mat4& previous, const glm::mat4& current, double alpha) const;
  };

}  // namespace other

#endif  // OTHER_PHYSICS_PHYSICS_WORLD_PHYSICS_WORLD_HPP
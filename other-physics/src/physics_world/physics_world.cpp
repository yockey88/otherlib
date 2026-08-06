/**
 * \file physics_world/physics_world.cpp
 **/
#include "physics_world/physics_world.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

#include "physics/physics_environment.hpp"
#include "physics_world/physics_body.hpp"
#include "physics_world/physics_shape.hpp"

namespace other {

  physics_world::physics_world(scope<memory_pool<physics_body>>& physics_bodies, scope<memory_pool<physics_shape>>& physics_shapes)
      : physics_bodies(physics_bodies), physics_shapes(physics_shapes) {
    OTHER_ASSERT(physics_bodies != nullptr, "Physics bodies memory pool is null.");
    OTHER_ASSERT(physics_shapes != nullptr, "Physics shapes memory pool is null.");
  }

  physics_api::physics_render_debug_data physics_world::get_debug_render_data() const {
    PROFILE_SECTION("physics_world::get_debug_render_data");
    return physics_api()->get_debug_render_data(world_id, this);
  }

  void physics_world::initialize(natural_t id, const physics_world_config& config) {
    auto* phys_env = subsystem<physics_environment>::get();
    OTHER_ASSERT(phys_env != nullptr, "Physics environment is not initialized.");

    world_id = id;
    physics_api()->initialize_world(id, this, config);
  }

  void physics_world::shutdown() {
    auto* phys_env = subsystem<physics_environment>::get();
    OTHER_ASSERT(phys_env != nullptr, "Physics environment is not initialized.");

    physics_api()->shutdown_world(this);
  }

  void physics_world::start_simulation() {
    physics_api()->on_scene_start(world_id, this);
  }

  void physics_world::stop_simulation() {
    physics_api()->on_scene_stop(world_id, this);
  }

  void physics_world::step_simulation(double delta_time) {
    PROFILE_SECTION("physics_world::step_simulation");

    auto* phys_env = subsystem<physics_environment>::get();
    OTHER_ASSERT(phys_env != nullptr, "Physics environment is not initialized.");

    physics_api()->step_simulation(world_id, this, delta_time);
    /// pull the post-step poses into the previous/current buffer, exactly once per step;
    ///  interpolation between the buffered poses is a pure read and can run per frame
    physics_api()->update_active_transforms(world_id, this, delta_time);
  }

  void physics_world::drain_contacts(ostd::vector<contact_event>& out) {
    physics_api()->drain_contacts(world_id, this, out);
    for (contact_event& ev : out) {
      if (ev.type != contact_event::kEnd) {
        continue;
      }
      /// the backend cannot distinguish sensor separations; re-flag from the engine bodies
      physics_body* a = body_by_id(ev.body_a);
      physics_body* b = body_by_id(ev.body_b);
      if ((a != nullptr && a->applied_settings.is_trigger) || (b != nullptr && b->applied_settings.is_trigger)) {
        ev.type = contact_event::kTriggerEnd;
      }
    }
  }

  raycast_hit physics_world::cast_ray(const glm::vec3& origin, const glm::vec3& direction, float max_distance) {
    raycast_hit hit = physics_api()->cast_ray(world_id, this, origin, direction, max_distance);
    if (hit.hit) {
      physics_body* body = body_by_id(hit.body_id);
      hit.owner_object_id = body != nullptr ? body->owner_object_id : 0;
    }
    return hit;
  }

  integer_t physics_world::create_fixed_joint(physics_body* body_a, physics_body* body_b) {
    OTHER_ASSERT(body_a != nullptr && body_b != nullptr, "Cannot weld a null physics body.");
    return physics_api()->create_fixed_joint(world_id, this, body_a, body_b);
  }

  void physics_world::destroy_joint(integer_t joint_id) {
    physics_api()->destroy_joint(world_id, this, joint_id);
  }

  float physics_world::joint_reaction_force(integer_t joint_id, double step) {
    return physics_api()->joint_reaction_force(world_id, this, joint_id, step);
  }

  void physics_world::set_linear_velocity(physics_body* body, const glm::vec3& velocity) {
    OTHER_ASSERT(body != nullptr, "Cannot set velocity on a null physics body.");
    physics_api()->set_linear_velocity(world_id, this, body, velocity);
  }

  glm::vec3 physics_world::get_linear_velocity(physics_body* body) {
    OTHER_ASSERT(body != nullptr, "Cannot read velocity from a null physics body.");
    return physics_api()->get_linear_velocity(world_id, this, body);
  }

  void physics_world::set_angular_velocity(physics_body* body, const glm::vec3& velocity) {
    OTHER_ASSERT(body != nullptr, "Cannot set velocity on a null physics body.");
    physics_api()->set_angular_velocity(world_id, this, body, velocity);
  }

  glm::vec3 physics_world::get_angular_velocity(physics_body* body) {
    OTHER_ASSERT(body != nullptr, "Cannot read velocity from a null physics body.");
    return physics_api()->get_angular_velocity(world_id, this, body);
  }

  void physics_world::add_force(physics_body* body, const glm::vec3& force) {
    OTHER_ASSERT(body != nullptr, "Cannot apply force to a null physics body.");
    physics_api()->add_force(world_id, this, body, force);
  }

  void physics_world::add_impulse(physics_body* body, const glm::vec3& impulse) {
    OTHER_ASSERT(body != nullptr, "Cannot apply impulse to a null physics body.");
    physics_api()->add_impulse(world_id, this, body, impulse);
  }

  void physics_world::add_torque(physics_body* body, const glm::vec3& torque) {
    OTHER_ASSERT(body != nullptr, "Cannot apply torque to a null physics body.");
    physics_api()->add_torque(world_id, this, body, torque);
  }

  void physics_world::teleport_body(physics_body* body, const glm::mat4& world_transform) {
    OTHER_ASSERT(body != nullptr, "Cannot teleport a null physics body.");
    physics_api()->teleport_body(world_id, this, body, world_transform);
  }

  void physics_world::move_kinematic(physics_body* body, const glm::mat4& world_transform, double step) {
    OTHER_ASSERT(body != nullptr, "Cannot move a null physics body.");
    OTHER_ASSERT(body->body_type == physics_body::KINEMATIC, "move_kinematic requires a KINEMATIC body.");
    physics_api()->move_kinematic(world_id, this, body, world_transform, step);
  }

  void physics_world::interpolate_active_transforms(double alpha) {
    PROFILE_SECTION("physics_world::interpolate_active_transforms");
    for (auto& live_obj : live_objects) {
      if (live_obj.object == nullptr || !live_obj.object->active) {
        continue;
      }

      live_obj.object->interpolated_transform = interpolate_transform(live_obj.object->previous_transform, live_obj.object->current_transform, alpha);
    }
  }

  physics_body* physics_world::create_physics_body(const physics_body::settings& settings, const glm::mat4& world_transform) {
    OTHER_ASSERT(physics_bodies != nullptr, "Physics body memory pool is not initialized.");
    if (physics_bodies->full()) {
      CORE_LOG_ERROR("Physics body pool is full ({} bodies), cannot create another.", kMaxPhysicsBodies);
      return nullptr;
    }
    auto [body, idx] = physics_bodies->emplace();

    live_body& live_obj = live_objects[idx];
    OTHER_ASSERT(live_obj.object == nullptr, "Physics body at index {} is already allocated.", idx);

    live_obj.id = idx;
    live_obj.object = &body;
    live_obj.object->id = static_cast<integer_t>(idx);
    live_obj.object->body_type = static_cast<physics_body::type>(settings.body_type);
    live_obj.object->mass = settings.mass;
    live_obj.object->applied_settings = settings;

    live_obj.object->previous_transform = world_transform;
    live_obj.object->current_transform = world_transform;
    live_obj.object->interpolated_transform = world_transform;

    physics_api()->register_physics_body(world_id, this, live_obj.object);

    CORE_LOG_DEBUG("Created physics body [{}:{}] with ID {}", name, idx, idx);
    return live_obj.object;
  }

  void physics_world::destroy_physics_body(physics_body* body) {
    OTHER_ASSERT(body != nullptr, "Cannot destroy a null physics body.");
    size_t idx = static_cast<size_t>(body->id);
    OTHER_ASSERT(idx < kMaxPhysicsBodies, "Physics body ID {} is out of bounds.", body->id);

    live_body& live_obj = live_objects[idx];
    OTHER_ASSERT(live_obj.object == body, "Physics body at index {} does not match the provided body.", idx);

    physics_api()->unregister_physics_body(world_id, this, live_obj.object);

    physics_bodies->free(idx);
    live_obj.id = 0;
    live_obj.object = nullptr;

    CORE_LOG_DEBUG("Destroyed physics body [{}:{}] with ID {}", name, idx, idx);
  }

  namespace {

    bounding_box scaled_geometry_bounds(const shape_geometry& geometry, const glm::vec3& world_scale) {
      bounding_box bounds = bounding_box::empty;
      for (const glm::vec3& p : geometry.positions) {
        glm::vec3 sp = p * world_scale;
        bounds.min = glm::min(bounds.min, sp);
        bounds.max = glm::max(bounds.max, sp);
      }
      return bounds;
    }

  }  // namespace

  physics_shape* physics_world::apply_shape(physics_body* body, const physics_shape_desc& desc,
                                            const glm::vec3& world_scale, const shape_geometry* geometry,
                                            const bounding_box* fit_bounds) {
    OTHER_ASSERT(body != nullptr, "Cannot apply a shape to a null physics body.");

    live_body& live_obj = live_objects[static_cast<size_t>(body->id)];
    OTHER_ASSERT(live_obj.object == body, "Physics body at index {} does not match the provided body.", body->id);

    if (live_obj.shape == nullptr) {
      auto [shape, idx] = physics_shapes->emplace();
      shape.id = static_cast<integer_t>(idx);
      shape.body_id = body->id;
      shape.applied.shape_kind = NUM_PHYSICS_SHAPE_KINDS;  /// never-built sentinel: first pass always builds
      body->shape_id = shape.id;
      live_obj.shape = &shape;
      CORE_LOG_DEBUG("Created physics shape [{}:{}] with ID {} for body ID {}", name, idx, idx, body->id);
    }
    physics_shape* shape = live_obj.shape;

    physics_shape_desc build_desc = desc;
    if (build_desc.shape_kind == PHYSICS_SHAPE_TRIANGLE_MESH && body->body_type != physics_body::STATIC) {
      CORE_LOG_WARN("Physics body {} is not static, downgrading triangle-mesh collider to a convex hull.", body->id);
      build_desc.shape_kind = PHYSICS_SHAPE_CONVEX_HULL;
    }
    if (build_desc.fit_render_bounds) {
      if (fit_bounds == nullptr) {
        return shape;  /// model bounds not available yet — revalidation retries
      }
      /// symmetric fit around the model origin; exact off-center boxes need compound shapes
      build_desc.half_extents = glm::max(glm::abs(fit_bounds->min), glm::abs(fit_bounds->max));
    }

    if (!physics_api()->set_body_shape(world_id, this, body, build_desc, world_scale, geometry)) {
      return shape;  /// build failed or deferred: applied untouched so revalidation retries
    }

    /// record the AUTHORED desc (even when the build downgraded) so the dirty-check settles
    shape->applied = desc;
    shape->applied_scale = world_scale;
    switch (build_desc.shape_kind) {
      case PHYSICS_SHAPE_CONVEX_HULL:
      case PHYSICS_SHAPE_TRIANGLE_MESH:
        OTHER_ASSERT(geometry != nullptr, "Geometry-backed shape built without geometry.");
        shape->local_bounds = scaled_geometry_bounds(*geometry, world_scale);
        break;
      default:
        shape->local_bounds = bounding_box::empty;  /// analytic kinds compute bounds on demand
        break;
    }
    return shape;
  }

  void physics_world::destroy_physics_shape(physics_shape* shape) {
    OTHER_ASSERT(shape != nullptr, "Cannot destroy a null physics shape.");

    size_t idx = static_cast<size_t>(shape->id);
    size_t bidx = static_cast<size_t>(shape->body_id);
    OTHER_ASSERT(idx < physics_shapes->kMaxSize, "Physics shape ID {} is out of bounds.", shape->id);
    OTHER_ASSERT(bidx < kMaxPhysicsBodies, "Physics body ID {} is out of bounds.", shape->body_id);

    live_body& live_obj = live_objects[bidx];
    OTHER_ASSERT(live_obj.shape == shape, "Physics shape at index {} does not match the provided shape.", idx);

    physics_shapes->free(idx);
    live_obj.shape = nullptr;
    live_obj.object->shape_id = -1;

    CORE_LOG_DEBUG("Destroyed physics shape [{}:{}] with ID {}", name, idx, idx);
  }

  scope<physics_api>& physics_world::physics_api() {
    auto* phys_env = subsystem<physics_environment>::get();
    OTHER_ASSERT(phys_env != nullptr, "Physics environment is not initialized.");
    return phys_env->api();
  }

  const scope<class physics_api>& physics_world::physics_api() const {
    auto* phys_env = subsystem<physics_environment>::get();
    OTHER_ASSERT(phys_env != nullptr, "Physics environment is not initialized.");
    return phys_env->api();
  }

  glm::mat4 physics_world::interpolate_transform(const glm::mat4& previous, const glm::mat4& current, double alpha) const {
    const float falpha = static_cast<float>(alpha);
    glm::vec3 inter_pos = glm::mix(glm::vec3(previous[3]), glm::vec3(current[3]), falpha);
    glm::quat inter_rot = glm::slerp(glm::quat_cast(previous), glm::quat_cast(current), falpha);

    glm::vec3 prev_scale = glm::vec3(
      glm::length(glm::vec3(previous[0])),
      glm::length(glm::vec3(previous[1])),
      glm::length(glm::vec3(previous[2])));
    glm::vec3 curr_scale = glm::vec3(
      glm::length(glm::vec3(current[0])),
      glm::length(glm::vec3(current[1])),
      glm::length(glm::vec3(current[2])));
    glm::vec3 inter_scale = glm::mix(prev_scale, curr_scale, falpha);

    glm::mat4 translation_mat = glm::translate(glm::mat4(1.0f), inter_pos);
    glm::mat4 rotation_mat = glm::mat4_cast(inter_rot);
    glm::mat4 scale_mat = glm::scale(glm::mat4(1.0f), inter_scale);

    return translation_mat * rotation_mat * scale_mat;
  }

}  // namespace other
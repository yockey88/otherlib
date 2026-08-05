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

  physics_body* physics_world::create_physics_body(const physics_body::settings& settings) {
    OTHER_ASSERT(physics_bodies != nullptr, "Physics body memory pool is not initialized.");
    auto [body, idx] = physics_bodies->emplace();

    live_body& live_obj = live_objects[idx];
    OTHER_ASSERT(live_obj.object == nullptr, "Physics body at index {} is already allocated.", idx);

    live_obj.id = idx;
    live_obj.object = &body;
    live_obj.object->id = static_cast<integer_t>(idx);
    live_obj.object->body_type = settings.body_type;
    live_obj.object->mass = settings.mass;

    live_obj.object->previous_transform = settings.world_transform;
    live_obj.object->current_transform = settings.world_transform;
    live_obj.object->interpolated_transform = settings.world_transform;

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

  physics_shape* physics_world::create_empty_shape(physics_body* body) {
    OTHER_ASSERT(body != nullptr, "Cannot create a shape for a null physics body.");

    /// this could be called more than once rn, might make this hard failure later
    {
      size_t idx = static_cast<size_t>(body->id);
      if (body->shape_id >= 0) {
        if (live_objects[idx].shape != nullptr) {
          return live_objects[idx].shape;
        }
      }
    }

    /// if there is no shape, attach an empty one
    auto [shape, idx] = physics_shapes->emplace();
    shape.id = static_cast<integer_t>(idx);
    body->shape_id = shape.id;

    live_body& live_obj = live_objects[static_cast<size_t>(body->id)];
    live_obj.shape = &shape;
    live_obj.shape->body_id = body->id;

    physics_api()->attach_shape(world_id, this, live_obj.object, live_obj.shape);

    CORE_LOG_DEBUG("Created empty physics shape [{}:{}] with ID {} for body ID {}", name, idx, idx, body->id);
    return &shape;
  }

  physics_shape* physics_world::create_box_shape(physics_body* body, const glm::vec3& half_extents) {
    OTHER_ASSERT(body != nullptr, "Cannot create a box shape for a null physics body.");

    physics_shape* shape = create_empty_shape(body);
    physics_api()->configure_box_shape(shape, half_extents);
    return shape;
  }

  physics_shape* physics_world::create_sphere_shape(physics_body* body, float radius) {
    OTHER_ASSERT(body != nullptr, "Cannot create a box shape for a null physics body.");
    CORE_LOG_ERROR("Sphere shape creation not yet implemented.");
    // physics_shape* shape = create_empty_shape(body);
    // physics_api()->configure_sphere_shape(shape, radius);
    return nullptr;
  }

  physics_shape* physics_world::create_capsule_shape(physics_body* body, float radius, float height) {
    OTHER_ASSERT(body != nullptr, "Cannot create a box shape for a null physics body.");
    CORE_LOG_ERROR("Capsule shape creation not yet implemented.");
    // physics_shape* shape = create_empty_shape(body);
    // physics_api()->configure_capsule_shape(shape, radius, height);
    return nullptr;
  }

  physics_shape* physics_world::create_convex_hull_shape(physics_body* body, const std::span<const glm::vec3> points) {
    OTHER_ASSERT(body != nullptr, "Cannot create a box shape for a null physics body.");
    CORE_LOG_ERROR("Convex hull shape creation not yet implemented.");
    // physics_shape* shape = create_empty_shape(body);
    // physics_api()->configure_convex_hull_shape(shape, points);
    return nullptr;
  }

  physics_shape* physics_world::create_triangle_mesh_shape(physics_body* body, const std::span<const glm::vec3> vertices, const std::span<const natural_t> indices) {
    OTHER_ASSERT(body != nullptr, "Cannot create a box shape for a null physics body.");
    CORE_LOG_ERROR("Triangle mesh shape creation not yet implemented.");
    // physics_shape* shape = create_empty_shape(body);
    // physics_api()->configure_triangle_mesh_shape(shape, vertices, indices);
    return nullptr;
  }

  physics_shape* physics_world::create_heightfield_shape(physics_body* body, const std::span<const float> height_data, natural_t width, natural_t depth, float min_height, float max_height) {
    OTHER_ASSERT(body != nullptr, "Cannot create a box shape for a null physics body.");
    CORE_LOG_ERROR("Heightfield shape creation not yet implemented.");
    // physics_shape* shape = create_empty_shape(body);
    // physics_api()->configure_heightfield_shape(shape, height_data, width, depth, min_height, max_height);
    return nullptr;
  }

  void physics_world::destroy_physics_shape(physics_shape* shape) {
    OTHER_ASSERT(shape != nullptr, "Cannot destroy a null physics shape.");

    size_t idx = static_cast<size_t>(shape->id);
    size_t bidx = static_cast<size_t>(shape->body_id);
    OTHER_ASSERT(idx < physics_shapes->kMaxSize, "Physics shape ID {} is out of bounds.", shape->id);
    OTHER_ASSERT(bidx < kMaxPhysicsBodies, "Physics body ID {} is out of bounds.", shape->body_id);

    live_body& live_obj = live_objects[bidx];
    OTHER_ASSERT(live_obj.shape == shape, "Physics shape at index {} does not match the provided shape.", idx);

    physics_api()->detach_shape(world_id, this, live_obj.object, live_obj.shape);
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
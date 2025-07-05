/**
 * \file renderer/camera.cpp
 **/
#include "renderer/camera.hpp"

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>

#include "core/profiler.hpp"
#include "math/constants.hpp"

#include "renderer/gpu_structs.hpp"

#include "glm/fwd.hpp"


namespace other {

  glm::vec3 camera::center() const {
    return position;
  }

  float camera::yaw() const {
    return euler_angles.x;
  }

  float camera::pitch() const {
    return euler_angles.y;
  }

  float camera::roll() const {
    return euler_angles.z;
  }

  glm::vec3 camera::orientation() const {
    return euler_angles;
  }

  glm::vec3 camera::forward() const {
    return basis.k;
  }

  glm::vec3 camera::up() const {
    return basis.j;
  }

  glm::vec3 camera::right() const {
    return basis.i;
  }

  const orthonormal_basis& camera::get_basis() const {
    return basis;
  }

  void camera::look_from(const glm::vec3& pos) {
    look(pos, pos + direction);
  }

  void camera::look_at(const glm::vec3& target) {
    look(position, target);
  }

  void camera::look(const glm::vec3& position, const glm::vec3& target) {
    this->position = position;
    direction = glm::normalize(target - position);

    basis = orthonormal_basis(world_up, direction);
    euler_angles = basis.to_local(euler_angles);

    reset_camera();
  }

  void camera::adjust_look_orientation(real_t yaw_adjust, real_t pitch_adjust) {
    PROFILE_SECTION("camera::adjust_look_orientation");
    euler_angles.x = yaw() + (yaw_adjust * sensitivity);
    euler_angles.y = pitch() - (pitch_adjust * sensitivity);
    if (constrain_pitch) {
      if (pitch() > 89.0f) {
        euler_angles.y = 89.0f;
      }

      if (pitch() < -89.0f) {
        euler_angles.y = -89.0f;
      }
    }

    glm::vec3 new_dir;
    new_dir.x = other::satisfy_floating_point_tolerance(cos(glm::radians(yaw())) * cos(glm::radians(pitch())));
    new_dir.y = other::satisfy_floating_point_tolerance(sin(glm::radians(pitch())));
    new_dir.z = other::satisfy_floating_point_tolerance(sin(glm::radians(yaw())) * cos(glm::radians(pitch())));
    direction = glm::normalize(new_dir);
    basis = other::orthonormal_basis(world_up, direction);
  }

  glm::mat4& camera::get_view_matrix() {
    view_matrix = glm::lookAt(position, position + direction, up());
    return view_matrix;
  }

  glm::mat4& camera::get_projection_matrix(const glm::ivec2& window_size) {
    projection_matrix = glm::perspective(glm::radians(fov), static_cast<real_t>(window_size.x) / window_size.y, clip.near_plane, clip.far_plane);
    return projection_matrix;
  }

  gpu::camera_data camera::to_gpu_data() {
    gpu::camera_data cam_data;
    cam_data.position = glm::vec4(position, 1.f);
    cam_data.forward = glm::vec4(direction, 0.f);
    cam_data.camera_features = glm::vec4(clip.near_plane, clip.far_plane, defocus_angle, 0.f);
    cam_data.defocus_disk_u = glm::vec4(defocus_disk_u, 0.f);
    cam_data.defocus_disk_v = glm::vec4(defocus_disk_v, 0.f);
    cam_data.view_matrix = get_view_matrix();
    cam_data.projection_matrix = get_projection_matrix(image_size);

    return cam_data;
  }

  gpu::ray_gen_data camera::to_ray_gen_data() {
    reset_camera();

    gpu::ray_gen_data ray_data;
    ray_data.pixel00_loc = glm::vec4(pixel00_loc, 0.f);
    ray_data.pixel_delta_u = glm::vec4(pixel_delta_u, 0.f);
    ray_data.pixel_delta_v = glm::vec4(pixel_delta_v, 0.f);

    return ray_data;
  }

  void camera::reset_camera() {
    image_size.x = image_width;
    image_size.y = int(image_size.x / aspect_ratio);
    image_size.y = (image_size.y < 1) ? 1 : image_size.y;

    pixel_samples_scale = 1.f / real_t(samples_per_pixel);

    real_t theta = degrees_to_radians(fov);
    real_t h = std::tan(theta / 2);
    real_t viewport_height = 2 * h * focus_dist;
    real_t viewport_width = viewport_height * (real_t(image_size.x) / image_size.y);

    glm::vec3 viewport_u = viewport_width * right();
    glm::vec3 viewport_v = viewport_height * -up();

    pixel_delta_u = viewport_u / real_t(image_size.x);
    pixel_delta_v = viewport_v / real_t(image_size.y);

    glm::vec3 viewport_upper_left = position - (focus_dist * forward()) - viewport_u / 2.f - viewport_v / 2.f;
    pixel00_loc = viewport_upper_left + 0.5f * (pixel_delta_u + pixel_delta_v);

    real_t defocus_radius = focus_dist * std::tan(degrees_to_radians(defocus_angle / 2));
    defocus_disk_u = right() * defocus_radius;
    defocus_disk_v = up() * defocus_radius;
  }

}  // namespace other
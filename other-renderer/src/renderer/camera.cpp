/**
 * \file renderer/camera.cpp
 **/
#include "renderer/camera.hpp"

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>

#include "math/constants.hpp"

#include "renderer/gpu_structs.hpp"

namespace other {

  glm::vec3 camera::center() const {
    return position;
  }

  glm::vec3 camera::direction() const {
    return center() - target;
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
    OTHER_ASSERT(basis.k == glm::normalize(direction()), "Camera basis k vector is not normalized to the camera direction.");
    return basis.k;
  }

  glm::vec3 camera::backward() const {
    return -basis.k;
  }

  glm::vec3 camera::up() const {
    return basis.j;
  }

  glm::vec3 camera::down() const {
    return -basis.j;
  }

  glm::vec3 camera::right() const {
    return basis.i;
  }

  glm::vec3 camera::left() const {
    return -basis.i;
  }

  const orthonormal_basis& camera::get_basis() const {
    return basis;
  }

  void camera::look_from(const glm::vec3& position) {
    this->position = position;
    reset_camera();
  }

  void camera::look_at(const glm::vec3& target) {
    this->target = target;
    reset_camera();
  }

  void camera::look(const glm::vec3& position, const glm::vec3& target) {
    this->position = position;
    this->position = target;
    reset_camera();
  }

  void camera::look() {
    reset_camera();
  }

  void camera::adjust_yaw(real_t angle) {
    euler_angles.x -= angle * sensitivity;
  }

  void camera::adjust_pitch(real_t angle) {
    euler_angles.y += angle * sensitivity;
    euler_angles.y = glm::clamp(euler_angles.y, -89.f, 89.f);  // Prevent gimbal lock
  }

  glm::mat4& camera::get_view_matrix() {
    view_matrix = glm::lookAt(position, target, up());
    return view_matrix;
  }

  glm::mat4& camera::get_projection_matrix(const glm::ivec2& window_size) {
    projection_matrix = glm::perspective(glm::radians(fov), static_cast<real_t>(window_size.x) / window_size.y, clip.near_plane, clip.far_plane);
    return projection_matrix;
  }

  gpu::camera_data camera::to_gpu_data() {
    reset_camera();

    gpu::camera_data cam_data;
    cam_data.position = glm::vec4(position, 1.f);
    cam_data.forward = glm::vec4(glm::normalize(target - position), 0.f);
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

  camera_specification camera::to_specification(const camera& cam) {
    return {
      .position = cam.position,
      .target = cam.target,
      .orientation = cam.orientation(),

      .world_up = cam.world_up,
      .image_size = cam.image_size,

      .fov = cam.fov,
      .aspect_ratio = cam.aspect_ratio,
      .defocus_angle = cam.defocus_angle,
      .focus_dist = cam.focus_dist,
      .image_width = cam.image_width,
      .sensitivity = cam.sensitivity,

      .samples_per_pixel = cam.samples_per_pixel,
      .max_bounce_depth = cam.max_bounce_depth,

      .constrain_pitch = cam.constrain_pitch,
    };
  }

  camera camera::from_specification(const camera_specification& spec) {
    camera cam;
    cam.position = spec.position;
    cam.target = spec.target;
    cam.world_up = spec.world_up;
    cam.euler_angles = spec.orientation;

    cam.image_size = spec.image_size;
    cam.fov = spec.fov;
    cam.aspect_ratio = spec.aspect_ratio;
    cam.defocus_angle = spec.defocus_angle;
    cam.focus_dist = spec.focus_dist;
    cam.image_width = spec.image_width;
    cam.sensitivity = spec.sensitivity;

    cam.samples_per_pixel = spec.samples_per_pixel;
    cam.max_bounce_depth = spec.max_bounce_depth;

    cam.constrain_pitch = spec.constrain_pitch;

    cam.reset_camera();
    return cam;
  }

  void camera::reset_camera() {
    glm::vec3 new_dir;
    new_dir.x = cos(glm::radians(yaw())) * cos(glm::radians(pitch()));
    new_dir.y = sin(glm::radians(pitch()));
    new_dir.z = sin(glm::radians(yaw())) * cos(glm::radians(pitch()));

    target = position + glm::normalize(new_dir);
    basis = orthonormal_basis(world_up, direction());

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
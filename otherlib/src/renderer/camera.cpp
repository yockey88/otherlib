/**
 * \file renderer/camera.cpp
 **/
#include "renderer/camera.hpp"

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>

#include "renderer/gpu_structs.hpp"

#include "math/constants.hpp"

namespace other {

  glm::vec3 camera::center() const {
    return position;
  }

  glm::vec3 camera::direction() const {
    return center() - target;
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

  glm::mat4& camera::get_view_matrix() {
    view_matrix = glm::lookAt(position, target, up());
    return view_matrix;
  }

  glm::mat4& camera::get_projection_matrix(const glm::ivec2& window_size) {
    projection_matrix = glm::perspective(glm::radians(fov), static_cast<float>(window_size.x) / window_size.y, clip.near_plane, clip.far_plane);
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

  void camera::reset_camera() {
    basis = orthonormal_basis(world_up, direction());

    image_size.x = image_width;
    image_size.y = int(image_size.x / aspect_ratio);
    image_size.y = (image_size.y < 1) ? 1 : image_size.y;

    pixel_samples_scale = 1.f / float(samples_per_pixel);

    float theta = degrees_to_radians(fov);
    float h = std::tan(theta / 2);
    float viewport_height = 2 * h * focus_dist;
    float viewport_width = viewport_height * (float(image_size.x) / image_size.y);

    glm::vec3 viewport_u = viewport_width * right();
    glm::vec3 viewport_v = viewport_height * -up();

    pixel_delta_u = viewport_u / float(image_size.x);
    pixel_delta_v = viewport_v / float(image_size.y);

    glm::vec3 viewport_upper_left = position - (focus_dist * forward()) - viewport_u / 2.f - viewport_v / 2.f;
    pixel00_loc = viewport_upper_left + 0.5f * (pixel_delta_u + pixel_delta_v);

    float defocus_radius = focus_dist * std::tan(degrees_to_radians(defocus_angle / 2));
    defocus_disk_u = right() * defocus_radius;
    defocus_disk_v = up() * defocus_radius;
  }

}  // namespace other
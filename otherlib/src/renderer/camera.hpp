/**
 * \file renderer/camera.hpp
 **/
#ifndef OTHER_RENDERER_CAMERA_HPP
#define OTHER_RENDERER_CAMERA_HPP

#include <glm/glm.hpp>

#include "renderer/gpu_structs.hpp"

#include "math/orthonormal_basis.hpp"

namespace other {

  class camera {
   public:
    struct clip_planes {
      float near_plane = 0.01f;
      float far_plane = 100.f;
    } clip;

    glm::vec3 center() const;
    glm::vec3 direction() const;

    /// orthonormal basis vectors
    glm::vec3 forward() const;
    glm::vec3 up() const;
    glm::vec3 right() const;

    void look_from(const glm::vec3& position);
    void look_at(const glm::vec3& target);
    void look(const glm::vec3& position, const glm::vec3& target);
    void look();

    glm::mat4& get_view_matrix();
    glm::mat4& get_projection_matrix(const glm::ivec2& window_size);

    gpu::camera_data to_gpu_data();
    gpu::ray_gen_data to_ray_gen_data();

    glm::vec3 world_up = glm::vec3(0.f, 1.f, 0.f);
    glm::vec3 euler_angles = glm::vec3(0.f, 0.f, 0.f);

    glm::vec2 image_size;

    float defocus_angle = 0;
    float focus_dist = 10;

    glm::vec3 defocus_disk_u;  /// horizontal
    glm::vec3 defocus_disk_v;  // vertical

    float image_width = 2.f;
    float aspect_ratio = 16.f / 9.f;
    int samples_per_pixel = 100;
    int max_bounce_depth = 50;
    float fov;

    float sensitivity = 0.1f;
    bool constrain_pitch = true;

   private:
    glm::vec3 position = glm::vec3(0.f, 0.f, 1.f);
    glm::vec3 target = glm::vec3(0.f, 0.f, 0.f);
    orthonormal_basis basis;

    glm::vec3 viewport_upper_left;

    glm::vec3 viewport_u;
    glm::vec3 viewport_v;

    glm::mat4 view_matrix;
    glm::mat4 projection_matrix;

    float pixel_samples_scale = 1.f;
    glm::vec3 pixel00_loc;
    glm::vec3 pixel_delta_u;
    glm::vec3 pixel_delta_v;

    void reset_camera();
  };

}  // namespace other

#endif  // OTHER_RENDERER_CAMERA_HPP
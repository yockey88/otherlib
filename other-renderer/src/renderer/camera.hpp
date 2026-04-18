/**
 * @file renderer/camera.hpp
 */
#ifndef OTHER_RENDERER_RENDERER_CAMERA_HPP
#define OTHER_RENDERER_RENDERER_CAMERA_HPP

#include <glm/glm.hpp>

#include "core/defines.hpp"
#include "math/frustum.hpp"
#include "math/orthonormal_basis.hpp"
#include "serialization/reflection.hpp"

#include "renderer/gpu_structs.hpp"

#include "glm/fwd.hpp"

namespace other {

  class camera {
   public:
    struct clip_planes {
      real_t near_plane = 0.01f;
      real_t far_plane = 100.f;
    } clip;

    void set_viewport_size(const glm::vec2& size);

    glm::vec3 center() const;

    float yaw() const;
    float pitch() const;
    float roll() const;
    glm::vec3 orientation() const;

    /// orthonormal basis vectors
    glm::vec3 forward() const;
    glm::vec3 up() const;
    glm::vec3 right() const;
    const orthonormal_basis& get_basis() const;

    void look_from(const glm::vec3& position);
    void look_at(const glm::vec3& target);
    void look(const glm::vec3& position, const glm::vec3& target);

    void adjust_look_orientation(real_t yaw, real_t pitch);

    void calculate_matrices(const glm::ivec2& window_size);
    glm::mat4& get_view_matrix();
    glm::mat4& get_projection_matrix(const glm::ivec2& window_size);

    gpu::camera_data to_gpu_data();
    gpu::ray_gen_data to_ray_gen_data();

    glm::vec3 position = glm::vec3(0.f, 0.f, 1.f);
    glm::vec3 direction = glm::vec3(0.f, 0.f, -1.f);
    glm::vec3 euler_angles = glm::vec3(0.f, 0.f, 0.f);
    glm::vec3 world_up = glm::vec3(0.f, 1.f, 0.f);
    orthonormal_basis basis = orthonormal_basis(glm::vec3(0.f, 0.f, -1.f), glm::vec3(0.f, 1.f, 0.f));

    glm::vec2 image_size = { 1920.f, 1080.f };  /// width, height

    real_t defocus_angle = 0;
    real_t focus_dist = 10;

    glm::vec3 defocus_disk_u = { 0.f, 0.f, 0.f };  /// horizontal
    glm::vec3 defocus_disk_v = { 0.f, 0.f, 0.f };  // vertical

    real_t image_width = 2.f;
    real_t aspect_ratio = 16.f / 9.f;
    int samples_per_pixel = 100;
    int max_bounce_depth = 50;
    real_t fov = 90.f;

    real_t sensitivity = 0.1f;
    bool constrain_pitch = true;

   private:
    glm::vec3 viewport_upper_left = glm::vec3(0.f, 0.f, 0.f);

    glm::vec3 viewport_u = glm::vec3(0.f, 0.f, 0.f);
    glm::vec3 viewport_v = glm::vec3(0.f, 0.f, 0.f);

    glm::mat4 view_matrix = glm::mat4(1.f);
    glm::mat4 projection_matrix = glm::mat4(1.f);

    real_t pixel_samples_scale = 1.f;
    glm::vec3 pixel00_loc = glm::vec3{ 0.f, 0.f, 0.f };
    glm::vec3 pixel_delta_u = glm::vec3{ 0.f, 0.f, 0.f };
    glm::vec3 pixel_delta_v = glm::vec3{ 0.f, 0.f, 0.f };

    frustum cam_frustum;

    void reset_camera();
  };

}  // namespace other

OTHER_REFLECT(
  other::camera,
  field(position, other::attr::serializable()),
  field(direction, other::attr::serializable()),
  field(euler_angles, other::attr::serializable()),
  field(world_up, other::attr::serializable()),
  field(basis, other::attr::serializable()),

  field(defocus_angle, other::attr::serializable()),
  field(focus_dist, other::attr::serializable()),

  field(image_width, other::attr::serializable()),
  field(aspect_ratio, other::attr::serializable()),
  field(samples_per_pixel, other::attr::serializable()),
  field(max_bounce_depth, other::attr::serializable()),
  field(fov, other::attr::serializable()),

  field(sensitivity, other::attr::serializable()),
  field(constrain_pitch, other::attr::serializable())
)

#endif  // OTHER_RENDERER_RENDERER_CAMERA_HPP
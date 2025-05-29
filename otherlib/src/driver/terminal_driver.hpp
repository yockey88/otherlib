/**
 * \file driver/terminal_driver.hpp
 **/
#ifndef OTHER_DRIVER_TERMINAL_DRIVER_HPP
#define OTHER_DRIVER_TERMINAL_DRIVER_HPP

#include "driver/driver.hpp"
#include "renderer/renderer_resource.hpp"

#include "glm/fwd.hpp"
#include "math/ray.hpp"
#include "simulation/scene_object.hpp"

namespace other {

  struct camera {
    struct {
      float near_plane = 0.01f;
      float far_plane = 100.f;
    } clip;

    glm::vec3 position;
    glm::vec3 target;

    glm::vec3 world_up = glm::vec3(0.f, 1.f, 0.f);
    glm::vec3 up = glm::vec3(0.f, 1.f, 0.f);
    float fov;

    bool constrain_pitch = true;
    float sensitivity = 0.1f;
    glm::vec3 euler_angles = glm::vec3(0.f, 0.f, 0.f);

    glm::vec3 forward() const {
      return glm::normalize(target - position);
    }

    glm::mat4& get_view_matrix();
    glm::mat4& get_projection_matrix(const glm::ivec2& window_size);

   private:
    glm::mat4 view_matrix;
    glm::mat4 projection_matrix;
  };

  struct mouse_state {
    glm::vec2 position = { 0, 0 };
    glm::vec2 delta = { 0, 0 };
  };

  class terminal_driver : public driver {
   public:
    terminal_driver(const config_table& config)
        : driver(config) {}
    virtual ~terminal_driver() = default;

    void on_initialize() override;
    void run() override;
    void on_shutdown() override;

   private:
    bool running = true;
    camera cam;
    mouse_state mouse;

    scope<renderer> renderer = nullptr;

    resource_handle comp_shader_handle;
    resource_handle screen_shader_handle;
    resource_handle screen_texture_handle;
    resource_handle quad_mesh_handle;

    resource_handle camera_buffer_handle;
    resource_handle scene_metadata_handle;
    resource_handle ray_buffer_handle;

    resource_handle material_buffer_handle;
    resource_handle lambertian_buffer_handle;
    resource_handle metal_buffer_handle;
    resource_handle dielectrics_buffer_handle;

    resource_handle sphere_buffer_handle;
    resource_handle object_buffer_handle;

    ref<compound_object> scene = nullptr;

    constexpr static uint8_t kPixelStride = 4;

    float pixel_sample_scale = 0.f;
    uint32_t samples_per_pixel = 5;
    uint32_t max_depth = 50;
    float reflectance = 0.15f;

    float aspect_ratio = 16.f / 9.f;
    float image_width = 800.f;
    glm::ivec2 image_size;

    glm::vec3 pixel00_loc;
    glm::vec3 pixel_delta_u;
    glm::vec3 pixel_delta_v;

    const char* output_path = "artifacts/output.png";

    std::vector<uint8_t> image_data;

    void on_event(SDL_Event* event) override;

    ray get_ray(int32_t i, int32_t j) const;

    glm::vec3 ray_color(const ray& r, uint32_t depth, const scene_object& obj) const;
    void write_pixel(const glm::ivec2& pixel, const glm::vec3& color);
  };

  driver* create_terminal_driver(const config_table& config);
  void destroy_terminal_driver(driver* instance);

}  // namespace other

#endif  // OTHER_DRIVER_TERMINAL_DRIVER_HPP
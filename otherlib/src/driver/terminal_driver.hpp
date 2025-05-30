/**
 * \file driver/terminal_driver.hpp
 **/
#ifndef OTHER_DRIVER_TERMINAL_DRIVER_HPP
#define OTHER_DRIVER_TERMINAL_DRIVER_HPP

#include "driver/driver.hpp"
#include "renderer/camera.hpp"
#include "renderer/renderer_resource.hpp"

#include "glm/fwd.hpp"
#include "math/ray.hpp"
#include "simulation/scene_object.hpp"

namespace other {

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
    bool run_on_gpu = true;

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

    // ref<compound_object> scene = nullptr;

    constexpr static uint8_t kPixelStride = 4;

    glm::vec3 pixel00_loc;
    glm::vec3 pixel_delta_u;
    glm::vec3 pixel_delta_v;

    const char* output_path = "artifacts/output.png";

    std::vector<uint8_t> image_data;

    void on_event(SDL_Event* event) override;

    void initialize_gpu();
    void on_gpu();

    void initialize_cpu();
    void on_cpu();

    // ray get_ray(int32_t i, int32_t j) const;

    // glm::vec3 ray_color(const ray& r, uint32_t depth, const scene_object& obj) const;
    // void write_pixel(const glm::ivec2& pixel, const glm::vec3& color);
  };

  driver* create_terminal_driver(const config_table& config);
  void destroy_terminal_driver(driver* instance);

}  // namespace other

#endif  // OTHER_DRIVER_TERMINAL_DRIVER_HPP
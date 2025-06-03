// /**
//  * \file renderer_driver.hpp
//  **/
#ifndef OTHER_RENDERER_DRIVER_HPP
#define OTHER_RENDERER_DRIVER_HPP

#include "gpu_resource/renderer_resource.hpp"
#include "model/model.hpp"
#include "renderer/camera.hpp"

#include "scene/scene.hpp"

#include "driver/driver.hpp"
#include "object/transform.hpp"

// #include "glm/fwd.hpp"
// #include "math/ray.hpp"
// #include "simulation/scene_object.hpp"

namespace other {

  struct mouse_state {
    glm::vec2 position = { 0, 0 };
    glm::vec2 delta = { 0, 0 };
  };

  class OTHER_CLASS renderer_driver : public driver {
   public:
    renderer_driver(const config_table& config)
        : driver(config) {}
    virtual ~renderer_driver() = default;

    void on_initialize() override;
    void run() override;
    void on_shutdown() override;

   private:
    bool run_on_gpu = true;

    bool running = true;
    camera cam;
    mouse_state mouse;

    scope<renderer> renderer = nullptr;
    scene active_scene;

    natural_t cube_id;
    natural_t capsule_id;

    model cube;
    model capsule;

    resource_handle comp_shader_handle;
    resource_handle screen_shader_handle;
    resource_handle cube_shader;

    resource_handle initial_pass;
    resource_handle comp_pass;
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
  };

}  // namespace other

OTHER_DRIVER(other::renderer_driver)

#endif  // OTHER_RENDERER_DRIVER_HPP
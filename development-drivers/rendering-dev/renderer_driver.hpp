// /**
//  * \file renderer_driver.hpp
//  **/
#ifndef OTHER_RENDERER_DRIVER_HPP
#define OTHER_RENDERER_DRIVER_HPP

#include "core/defines.hpp"

#include "gpu_resource/renderer_resource.hpp"
#include "model/model.hpp"
#include "renderer/camera.hpp"
#include "renderer/render_pipeline.hpp"

#include "scene/scene.hpp"

#include "driver/driver.hpp"


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

    bool pressing_mouse_wheel = false;
    mouse_state mouse;

    scope<renderer> renderer = nullptr;
    scope<render_graph> frame_graph = nullptr;
    scope<render_pipeline> render_pipeline = nullptr;
    scene active_scene;

    natural_t light_id;
    natural_t suzanne_id;

    model cube;
    model suzanne;

    resource_handle initial_pass;
    resource_handle screen_texture_handle;

    resource_handle quad_mesh_handle;
    resource_handle screen_shader_handle;

    resource_handle instancing_shader;

    resource_handle camera_buffer_handle;

    resource_handle point_light_buffer_handle;
    resource_handle dir_light_buffer_handle;

    resource_handle material_buffer_handle;
    resource_handle model_buffer_handle;

    void on_event(SDL_Event* event) override;
  };

}  // namespace other

OTHER_DRIVER(other::renderer_driver)

#endif  // OTHER_RENDERER_DRIVER_HPP
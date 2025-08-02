/**
 * \file renderer_driver.hpp
 **/
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

    bool pressing_mouse_wheel = false;
    mouse_state mouse;

    scope<renderer> renderer = nullptr;
    scene active_scene;

    natural_t light_id;
    natural_t suzanne_id;
    natural_t camera_id;

    model cube;
    model suzanne;

    ref<assembly> other_assembly = nullptr;

    void on_event(SDL_Event* event) override;
  };

}  // namespace other

OTHER_DRIVER(other::renderer_driver)

#endif  // OTHER_RENDERER_DRIVER_HPP
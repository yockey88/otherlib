/**
 * \file simulation_driver.hpp
 **/
#ifndef OTHER_SIMULATION_DRIVER_HPP
#define OTHER_SIMULATION_DRIVER_HPP

#include "core/defines.hpp"

#include "scene/scene.hpp"

#include "driver/driver.hpp"

#include "simulation.hpp"

#include "sim-config-spec_generated.h"


namespace other {

  class OTHER_CLASS simulation_driver : public driver {
   public:
    simulation_driver(const config_table& config)
        : driver(config) {}
    virtual ~simulation_driver() = default;

    void on_initialize() override;
    void run() override;
    void on_shutdown() override;

   private:
    bool running = true;

    scope<renderer> renderer = nullptr;
    scene active_scene;

    natural_t simulation_id;

    std::vector<float> pixel_colors = {};

    resource_handle sim_texture;

    void on_event(SDL_Event* event) override;
  };

}  // namespace other

OTHER_DRIVER(other::simulation_driver)

#endif  // OTHER_SIMULATION_DRIVER_HPP
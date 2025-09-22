/**
 * \file runtime-dev/runtime.hpp
 **/
#ifndef OTHER_RUNTIME_HPP
#define OTHER_RUNTIME_HPP

#include "scene/scene_graph.hpp"

#include "driver/driver.hpp"

namespace other {

  class OTHER_CLASS runtime : public driver {
   public:
    runtime(const config_table& config)
        : driver(config) {}
    virtual ~runtime() = default;

    void on_initialize() override;
    void run() override;
    void on_shutdown() override;

   private:
    bool running = false;
    scene_graph scenes;

    ref<assembly> other_assembly = nullptr;
    ref<assembly> testing_assembly = nullptr;

    // void initialize_subsystems();
    // void load_project_configuration();
    // void load_scenes_and_play();

    void update();
    void draw();

    void on_event(SDL_Event* event) override;
  };

}  // namespace other

OTHER_DRIVER(other::runtime)

#endif  // OTHER_RUNTIME_HPP
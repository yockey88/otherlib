/**
 * \file runtime-dev/runtime.hpp
 **/
#ifndef OTHER_RUNTIME_HPP
#define OTHER_RUNTIME_HPP

#include "thread/message_bus.hpp"

#include "network/network_thread.hpp"

#include "scene/scene_graph.hpp"

#include "driver/driver.hpp"

namespace other {

  class OTHER_CLASS runtime : public driver {
   public:
    runtime(const config_table& config)
        : driver(config) {}
    virtual ~runtime() = default;

    void on_initialize(const command_line& cmd) override;
    void run() override;
    void on_shutdown() override;

    void catch_signal(int signal) override;

   private:
    bool running = false;
    scope<scene_graph> project_scene_graph = nullptr;

    message_bus net_thread_message_bus;
    scope<network_thread> net_thread = nullptr;

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
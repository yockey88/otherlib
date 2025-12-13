/**
 * \file runtime-dev/runtime.hpp
 **/
#ifndef OTHER_RUNTIME_HPP
#define OTHER_RUNTIME_HPP

#include <chrono>

#include "core/state_machine.hpp"
#include "event/event_system.hpp"
#include "thread/message_bus.hpp"

#include "network/network_thread.hpp"

#include "scene/scene_graph.hpp"

#include "driver/driver.hpp"
#include "ui/console.hpp"
#include "ui/node_editor.hpp"

#include "asset/asset_handler.hpp"
#include "runtime_ui.hpp"

namespace other {

  class OTHER_CLASS runtime : public driver {
   public:
    runtime(const config_table& config)
        : driver(config) {}
    virtual ~runtime() = default;

    void on_initialize(const command_line& cmd) override;
    void on_update() override;
    void on_shutdown() override;

   private:
    friend class runtime_state_machine;

    integer_t builder_obj_id = -1;

    float curr_frame_delta_time = 0.0f;
    std::chrono::steady_clock::time_point last_frame_time;

    scope<renderer> renderer = nullptr;
    scope<asset_handler> asset_mgr = nullptr;

    bool running = false;
    natural_t current_scene_id = 0;

    natural_t donut_id = 0;

    model donut_model;
    natural_t donut_model_id = 0;

    bool show_node_editor = true;
    scope<ui::node_editor> node_editor = nullptr;

    bool show_console_window = true;
    scope<ui::console_window> console_window = nullptr;
    lua_script* console_lua_script = nullptr;

    scope<runtime_control_window> runtime_ui = nullptr;

    void update_initializing() override;
    void on_initialize_ready() override;
    void update_running() override;
    void update_shutting_down() override;
    void draw();

    void handle_console_command(const std::string_view command, system_timepoint timestamp);
  };

}  // namespace other

OTHER_DRIVER(other::runtime)

#endif  // OTHER_RUNTIME_HPP
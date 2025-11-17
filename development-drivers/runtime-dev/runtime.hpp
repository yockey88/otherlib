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

#include "asset/asset_handler.hpp"
#include "runtime_ui.hpp"

namespace other {

  enum class runtime_state : natural_t {
    RUNTIME_STATE_SHUT_DOWN = 0,
    RUNTIME_STATE_LOADING_PROJECT,
    RUNTIME_STATE_WAITING_FOR_START_SCENE_LOAD,
    RUNTIME_STATE_RUNNING,
    RUNTIME_STATE_SHUTTING_DOWN,

    NUM_STATES,
  };
  enum class runtime_event : natural_t {
    RUNTIME_EVENT_START = 0,
    RUNTIME_EVENT_READY,
    RUNTIME_EVENT_STOP,
    RUNTIME_EVENT_SHUT_DOWN,

    NUM_EVENTS,
  };

  class runtime_state_machine : public state_machine<runtime_state, runtime_event> {
   public:
    runtime_state_machine()
        : state_machine<runtime_state, runtime_event>(runtime_state::RUNTIME_STATE_SHUT_DOWN) {
      add_transition(runtime_state::RUNTIME_STATE_SHUT_DOWN, runtime_event::RUNTIME_EVENT_START, runtime_state::RUNTIME_STATE_LOADING_PROJECT);

      add_transition(runtime_state::RUNTIME_STATE_LOADING_PROJECT, runtime_event::RUNTIME_EVENT_READY, runtime_state::RUNTIME_STATE_WAITING_FOR_START_SCENE_LOAD);
      add_transition(runtime_state::RUNTIME_STATE_LOADING_PROJECT, runtime_event::RUNTIME_EVENT_STOP, runtime_state::RUNTIME_STATE_SHUTTING_DOWN);

      add_transition(runtime_state::RUNTIME_STATE_WAITING_FOR_START_SCENE_LOAD, runtime_event::RUNTIME_EVENT_READY, runtime_state::RUNTIME_STATE_RUNNING);
      add_transition(runtime_state::RUNTIME_STATE_WAITING_FOR_START_SCENE_LOAD, runtime_event::RUNTIME_EVENT_STOP, runtime_state::RUNTIME_STATE_SHUTTING_DOWN);

      add_transition(runtime_state::RUNTIME_STATE_RUNNING, runtime_event::RUNTIME_EVENT_STOP, runtime_state::RUNTIME_STATE_SHUTTING_DOWN);

      add_transition(runtime_state::RUNTIME_STATE_SHUTTING_DOWN, runtime_event::RUNTIME_EVENT_STOP, runtime_state::RUNTIME_STATE_SHUT_DOWN);
    }
    virtual ~runtime_state_machine() = default;

    void on_enter_state(runtime_state new_state) override {
      CORE_LOG_DEBUG("Server state changed to {}", new_state);
    }
  };

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

    message_bus net_thread_message_bus;
    scope<network_thread> net_thread = nullptr;

    scope<event_system> events = nullptr;
    scope<runtime_control_window> runtime_ui = nullptr;
    runtime_state_machine state_machine;

    void core_update();
    void update_loading_project();
    void update_waiting_for_start_scene_load();
    void update_running();
    void update_shutting_down();
    void draw();

    void on_event(SDL_Event* event) override;
  };

}  // namespace other

OTHER_DRIVER(other::runtime)

#endif  // OTHER_RUNTIME_HPP
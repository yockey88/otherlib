/**
 * \file editor_driver.cpp
 **/
#include "editor_driver.hpp"

#include "core/timer.hpp"
#include "event/event_system.hpp"

#include "script/scripting_environment.hpp"

#include "rendering-pipelines/empty_pipeline.hpp"
#include "tools/environment_console.hpp"
#include "ui/console.hpp"

namespace other {

  void editor_driver::on_initialize(const command_line& cmd) {
    state_machine.handle_event(editor_event::EDITOR_EVENT_START);
    CORE_LOG_INFO("Initialized editor driver.");

    editor_lua_script = subsystem<scripting_environment>::get()->load_lua_file("resources/lua/editor.lua");
    environment_console::initialize(editor_lua_script);
    {
      logger* log = subsystem<logger>::get();
      log->create_logger("other-editor-log", spdlog::level::trace);
      log_sink console_log_sink = {
        .id = logger::get_next_sink_id(),
        .sink_name = "console-sink",
        .sink_pattern = "[%l] %v",
        .level = spdlog::level::info,
        .sink_factory = [](const config_table& config) -> spdlog::sink_ptr {
          return std::make_shared<console_sink_mt>();
        }
      };

      std::string loggers[] = { "other-editor-log", "other-core-log" };
      log->register_sink(loggers, console_log_sink);
    }

    {
      message connect_msg;
      connect_msg.header = {
        .category = COMMAND,
        .id = SESSION_CONNECT_TO,
      };
      const uint8_t* bp_bytes = reinterpret_cast<const uint8_t*>(&net_context->main_binding_point);
      connect_msg.data.append_range(std::span(bp_bytes, sizeof(binding_point)));
      net_context->net_thread_message_bus.send_message(std::move(connect_msg));
    }

    if (rendering_enabled()) {
      renderer = get_renderer();
      renderer->add_pipeline<empty_pipeline>("UI Pipeline");

      initialize_ui();
    }

    get_event_system()->add_listener("shutdown-requested", [this](const value& data) {
      if (state_machine.get_current_state() == editor_state::EDITOR_STATE_RUNNING) {
        state_machine.handle_event(editor_event::EDITOR_EVENT_STOP);
        net_context->net_thread->shutdown();
      }
    });

    get_event_system()->register_event("open-project");
    get_event_system()->register_event("edit-project");

    get_event_system()->add_listener("open-project", [this](const value& data) {
      if (data.type() != value_type::STRING) {
        CORE_LOG_ERROR("Invalid data type for open-project event. Expected string.");
        return;
      }

      std::string proj_name = data;
      CORE_LOG_INFO("Opening project: [{}]", proj_name);
    });
    get_event_system()->add_listener("edit-project", [this](const value& data) {
      if (data.type() != value_type::STRING) {
        CORE_LOG_ERROR("Invalid data type for edit-project event. Expected string.");
        return;
      }

      std::string proj_name = data;
      CORE_LOG_INFO("Editing project: [{}]", proj_name);
    });
  }

  void editor_driver::run() {
    CORE_LOG_INFO("Running editor driver...");

    while (state_machine.get_current_state() != editor_state::EDITOR_STATE_STOPPED) {
      core_update();

      switch (state_machine.get_current_state()) {
        case editor_state::EDITOR_STATE_INITIALIZING: update_initializing(); break;
        case editor_state::EDITOR_STATE_RUNNING: update_running(); break;
        case editor_state::EDITOR_STATE_SHUTTING_DOWN: update_shutting_down(); break;
        default:
          break;
      }

      if (rendering_enabled()) {
        renderer->begin_frame(nullptr);
        renderer->render();
        renderer->begin_ui_frame();
        ui_ptr->render();
        renderer->end_ui_frame();
        renderer->end_frame();
      }
    }
  }

  void editor_driver::on_shutdown() {
    CORE_LOG_INFO("Shut down editor driver.");

    if (rendering_enabled()) {
      shutdown_ui();
      renderer->remove_pipeline("UI Pipeline");
      renderer = nullptr;
    }
  }

  void editor_driver::core_update() {
    pump_events();
  }

  void editor_driver::update_initializing() {
    state_machine.handle_event(editor_event::EDITOR_EVENT_READY);
  }

  void editor_driver::update_running() {
  }

  void editor_driver::update_shutting_down() {
    if (net_context->net_thread->get_current_state() == thread::state::STOPPED) {
      state_machine.handle_event(editor_event::EDITOR_EVENT_READY);
    }
  }

  void editor_driver::on_event(SDL_Event* event) {
    switch (event->type) {
      case SDL_EVENT_QUIT:
      case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
        get_event_system()->trigger_event("shutdown-requested");
        break;
      default:
        break;
    }
  }

  void editor_driver::handle_notification_session_check_in(message&& msg) {
    OTHER_ASSERT(msg.data.size() >= sizeof(integer_t), "Invalid session check-in notification message size");

    integer_t session_id = *reinterpret_cast<const integer_t*>(msg.data.data());
    if (session_id < 1) {
      CORE_LOG_ERROR("Invalid session ID received in check-in notification: {}", session_id);
      return;
    }

    CORE_LOG_INFO("Session [{}] has checked in.", session_id);
    client_session_id = session_id;
  }

  void editor_driver::on_active_scene_udp_handle_bound(udp_handle* handle) {
    post_coroutine(active_scene_udp_loop());
  }

  task editor_driver::active_scene_udp_loop() {
    OTHER_ASSERT(get_active_scene() != nullptr, "No active scene to run UDP handle loop on.");
    CORE_LOG_INFO("Starting active scene UDP handle loop...");

    auto now = steady_clock::now();
    auto last = now;
    auto elapsed = now - last;
    while (state_machine.get_current_state() != editor_state::EDITOR_STATE_SHUTTING_DOWN && get_active_scene() != nullptr) {
      now = steady_clock::now();
      elapsed += now - last;
      last = now;

      if (elapsed >= seconds(3)) {
        elapsed = steady_clock::duration::zero();

        message udp_msg;
        udp_msg.header = {
          .category = NOTIFICATION,
          .id = PING,
        };
        std::string ping_str = "Ping from editor via UDP!";

        uint16_t size = static_cast<uint16_t>(ping_str.size());
        const uint8_t* size_bytes = reinterpret_cast<const uint8_t*>(&size);
        const uint8_t* ping_bytes = reinterpret_cast<const uint8_t*>(ping_str.data());
        udp_msg.data.append_range(std::span(size_bytes, sizeof(uint16_t)));
        udp_msg.data.append_range(std::span(ping_bytes, ping_str.size()));

        CORE_LOG_DEBUG("Sending UDP PING message to server: {}", ping_str);
        active_scene_udp_handle.send(std::move(udp_msg));
      } else {
        co_await task::awaiter{};
      }
    }
  }

  void editor_driver::initialize_ui() {
    ui_ptr = make_scope<editor_ui>(get_event_system());
    ui_ptr->initialize();

    get_event_system()->register_event("editor:main-menu:file:new-project");
    get_event_system()->add_listener("editor:main-menu:file:new-project", [this](const value& data) {
      CORE_LOG_INFO("New Project menu item selected.");
    });

    get_event_system()->register_event("editor:main-menu:file:open-project");
    get_event_system()->add_listener("editor:main-menu:file:open-project", [this](const value& data) {
      CORE_LOG_INFO("Open Project menu item selected.");
    });
  }

  void editor_driver::shutdown_ui() {
    ui_ptr->shutdown();
    ui_ptr = nullptr;
  }

}  // namespace other
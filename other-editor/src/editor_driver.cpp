/**
 * \file editor_driver.cpp
 **/
#include "editor_driver.hpp"

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

  void editor_driver::update_running() {}

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
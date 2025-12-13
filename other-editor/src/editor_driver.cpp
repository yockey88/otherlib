/**
 * \file editor_driver.cpp
 **/
#include "editor_driver.hpp"

#include "event/event_system.hpp"

#include "script/scripting_environment.hpp"

#include "rendering-pipelines/empty_pipeline.hpp"
#include "tools/environment_console.hpp"

namespace other {

  void editor_driver::on_initialize(const command_line& cmd) {
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

    /// ready : initializing -> running
    /// nothing to do right now for editor, should start in running state
    process_driver_event(driver_event::DRIVER_EVENT_READY);
  }

  void editor_driver::on_initialize_rendering(scope<renderer>& renderer_ptr) {
    renderer_ptr->add_pipeline<empty_pipeline>("UI Pipeline");
    initialize_ui();
  }

  void editor_driver::on_update() {
  }

  void editor_driver::on_ui_render() {
    ui_ptr->render();
  }

  void editor_driver::on_shutdown() {
    CORE_LOG_INFO("Shut down editor driver.");
  }

  void editor_driver::on_shutdown_rendering() {
    shutdown_ui();
    get_renderer_instance().remove_pipeline("UI Pipeline");
  }

  void editor_driver::update_running() {
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
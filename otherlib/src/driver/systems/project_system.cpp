/**
 * \file driver/systems/project_system.cpp
 **/
#include "driver/systems/project_system.hpp"

#include <toml++/toml.hpp>

#include "core/defines.hpp"

#include "driver/driver.hpp"

namespace other {
  namespace detail {

    /**
     * \note file dialog callbacks may be called from a different thread so the functions called must be thread safe
     **/

    void open_project_callback(void* userdata, const char* const* filelist, int32_t filter) {
      OTHER_ASSERT(userdata != nullptr, "User data is null in open project callback.");
      auto* driver = static_cast<other::driver*>(userdata);

      filepath project_file = std::string(filelist[0]);
      if (!std::filesystem::exists(project_file)) {
        CORE_LOG_ERROR("Selected project file '{}' does not exist.", project_file.string());
        return;
      }

      driver->queue_project_load(project_file);
    }

  }  // namespace detail

  void project_system::initialize(driver_kernel* kernel) {
    loaded_project = make_scope<project>(this);
    OTHER_ASSERT(loaded_project != nullptr, "Failed to create project instance");

    auto& events = *get_driver().get_event_system();
    events.register_event("project.loaded");

    events.register_event("project.new-project");
    events.add_listener("project.new-project", [this, kernel](const value& data) { handle_new_project(kernel, data); });

    events.register_event("project.open-project");
    events.add_listener("project.open-project", [this, kernel](const value& data) { handle_open_project(kernel, data); });

    events.register_event("project.save-project");
    events.add_listener("project.save-project", [this, kernel](const value& data) { handle_save_project(kernel, data); });

    const auto& config = get_driver().configuration();
    if (config.project_file.has_value()) {
      filepath project_file = *config.project_file;
      load_project(kernel, project_file);
    }
  }

  void project_system::tick(driver_kernel* kernel, double dt) {
  }

  void project_system::shutdown(driver_kernel* kernel) {
    OTHER_ASSERT(loaded_project != nullptr, "No project loaded in project system shutdown.");
    loaded_project->unload();
    loaded_project = nullptr;
  }

  bool project_system::project_empty() const {
    OTHER_ASSERT(loaded_project != nullptr, "No project loaded in project system.");
    return loaded_project->is_empty();
  }

  bool project_system::project_loading() const {
    OTHER_ASSERT(loaded_project != nullptr, "No project loaded in project system.");
    return loaded_project->is_loading();
  }

  bool project_system::project_loaded() const {
    OTHER_ASSERT(loaded_project != nullptr, "No project loaded in project system.");
    return loaded_project->is_loaded();
  }

  bool project_system::project_unloading() const {
    OTHER_ASSERT(loaded_project != nullptr, "No project loaded in project system.");
    return loaded_project->is_unloading();
  }

  void project_system::generate_project_at(driver_kernel* kernel, const filepath& directory) {
    OTHER_ASSERT(loaded_project != nullptr, "No project loaded in project system.");
  }

  void project_system::load_project(driver_kernel* kernel, const filepath& project_file) {
    OTHER_ASSERT(kernel != nullptr, "Kernel pointer is null in project system load_project.");
    OTHER_ASSERT(loaded_project != nullptr, "No project loaded in project system.");

    OTHER_ASSERT(std::filesystem::exists(project_file), "Project file '{}' does not exist.", project_file.string());
    OTHER_ASSERT(std::filesystem::is_regular_file(project_file), "Project file '{}' is not a regular file.", project_file.string());
    loaded_project->load_from_file(kernel, project_file);
    last_loaded_project_file = project_file;
  }

  void project_system::handle_new_project(driver_kernel* kernel, const value& data) {
    // get_driver().trigger_event("open-driver-ui-window", "project-creator");
  }

  void project_system::handle_open_project(driver_kernel* kernel, const value& data) {
    sibling<rendering_system>(*kernel).show_open_file_dialog(&detail::open_project_callback, &get_driver(), 0);
  }

  void project_system::handle_save_project(driver_kernel* kernel, const value& data) {
  }

}  // namespace other
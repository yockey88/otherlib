/**
 * \file driver/systems/project_system.cpp
 **/
#include "driver/systems/project_system.hpp"

#include <filesystem>

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

    events.register_event("native-project.load");
    events.add_listener("native-project.load", [this, kernel](const value& data) {
      if (data.type() != value_type::USER_TYPE) {
        CORE_LOG_ERROR("Invalid data type for native-project.load event. Expected USER_TYPE containing project file path.");
        return;
      }
      handle_project_event(kernel, data);
    });

    events.add_listener("script.asset-loaded", [this, kernel](const value& data) { handle_script_asset_loaded(kernel, data); });

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

  void project_system::unload_project() {
    OTHER_ASSERT(loaded_project != nullptr, "No project loaded in project system.");
    loaded_project->unload();
  }

  void project_system::handle_new_project(driver_kernel* kernel, const value& data) {
    // get_driver().trigger_event("open-driver-ui-window", "project-creator");
  }

  void project_system::handle_open_project(driver_kernel* kernel, const value& data) {
    sibling<rendering_system>(*kernel).show_open_file_dialog(&detail::open_project_callback, &get_driver(), 0);
  }

  void project_system::handle_save_project(driver_kernel* kernel, const value& data) {
  }

  void project_system::handle_project_event(driver_kernel* kernel, const project_event_data& data) {
    CORE_LOG_INFO("Received project event '{}' for project '{}' at path '{}'", data.type, data.project_name, data.project_path);

    std::string type = data.type;
    filepath project_path = data.project_path;
    if (type == "load") {
      if (!std::filesystem::exists(project_path)) {
        CORE_LOG_ERROR("Project file '{}' does not exist for project load event.", project_path.string());
        return;
      }
      if (std::filesystem::is_directory(project_path)) {
        /// \todo refactor this to something more robust to check lots of possible project files
        // check if there is a same-named .toml/.oproj file in the directory and use that as the project file, if not error out
        filepath expected_toml_project_file = project_path / (project_path.filename().string() + ".toml");
        filepath expected_oproj_project_file = project_path / (project_path.filename().string() + ".oproj");
        if (std::filesystem::exists(expected_toml_project_file) && std::filesystem::is_regular_file(expected_toml_project_file)) {
          project_path = expected_toml_project_file;
        } else if (std::filesystem::exists(expected_oproj_project_file) && std::filesystem::is_regular_file(expected_oproj_project_file)) {
          project_path = expected_oproj_project_file;
        } else {
          CORE_LOG_ERROR("Project path '{}' is a directory but no same-named .toml or .oproj file found in the directory for project load event.", project_path.string());
          return;
        }
      }

      load_project(kernel, project_path);
    } else if (type == "save") {
      handle_save_project(kernel, data);
    }
  }

  void project_system::handle_script_asset_loaded(driver_kernel* kernel, const value& data) {
    OTHER_ASSERT(loaded_project != nullptr, "No project loaded in project system.");
    OTHER_ASSERT(data.type() == value_type::UINT64, "Expected asset ID as uint64 in script asset loaded event data.");

    natural_t asset_id = data;
    filepath script = sibling<asset_system>(*kernel).get_local_asset_path(asset_id);
    if (script.empty()) {
      CORE_LOG_ERROR("Failed to get local asset path for loaded script asset with ID: {}", asset_id);
      return;
    }
    OTHER_ASSERT(std::filesystem::exists(script), "Local asset path '{}' for loaded script asset with ID {} does not exist.", script.string(), asset_id);

    loaded_project->add_built_script(script);
  }

}  // namespace other
/**
 * \file driver/systems/project_system.cpp
 **/
#include "driver/systems/project_system.hpp"

#include <toml++/toml.hpp>

#include "file/filesystem.hpp"

#include "dotnet/dotnet_object.hpp"
#include "script/scripting_environment.hpp"

#include "driver/driver.hpp"
#include "driver/systems/job_driver_system.hpp"
#include "tools/project_tool.hpp"

namespace other {

  void project_system::initialize(driver_kernel* kernel) {
    loaded_project = make_scope<project>(this);
    OTHER_ASSERT(loaded_project != nullptr, "Failed to create project instance");

    get_driver().get_event_system()->register_event("project.loaded");

    const auto& config = get_driver().configuration();
    if (config.project_file.has_value()) {
      filepath project_file = *config.project_file;
      // command line parser validates existence
      OTHER_ASSERT(std::filesystem::exists(project_file), "Project file '{}' does not exist.", project_file.string());
      OTHER_ASSERT(std::filesystem::is_regular_file(project_file), "Project file '{}' is not a regular file.", project_file.string());
      loaded_project->load_from_file(kernel, project_file);
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

}  // namespace other
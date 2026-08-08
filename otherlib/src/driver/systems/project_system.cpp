/**
 * \file driver/systems/project_system.cpp
 **/
#include "driver/systems/project_system.hpp"

#include <filesystem>

#include <toml++/toml.hpp>

#include "core/defines.hpp"

#include "script/scripting_environment.hpp"

#include "driver/driver.hpp"
#include "driver/systems/project_system.hpp"
#include "driver/systems/scene_system.hpp"

namespace other {
  namespace detail {

    /** \note file dialog callbacks may run off-thread; called functions must be thread safe
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
    PROFILE_SECTION("project_system::initialize");
    loaded_project = make_scope<project>(this);
    OTHER_ASSERT(loaded_project != nullptr, "Failed to create project instance");

    auto& events = *get_driver().get_event_system();
    events.register_event("project.loaded");
    events.register_event("project.unloaded");
    events.register_event("project.assembly-refreshed");
    /// the refresh detached every behavior whose type lived in the old assembly; rebuild
    ///  those instances from the freshly loaded one
    events.add_listener("project.assembly-refreshed", [](const value&) {
      auto* env = subsystem<scripting_environment>::get();
      OTHER_ASSERT(env != nullptr, "Scripting environment subsystem is not available.");
      env->reattach_invalidated_dotnet_behaviors();
    });

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
      handle_project_event(kernel, (project_event_data)data);
    });

    /// registered in asset_system::initialize
    events.add_listener("script-project.asset-loaded", [this, kernel](const value& data) { handle_script_project_loaded(kernel, data); });
    events.add_listener("script-project.asset-load-failed", [this, kernel](const value& data) { handle_script_project_load_failed(kernel, data); });
    events.add_listener("script-source.asset-loaded", [this, kernel](const value& data) { handle_script_source_loaded(kernel, data); });
    events.add_listener("script-source.asset-load-failed", [this, kernel](const value& data) { handle_script_source_load_failed(kernel, data); });
    events.add_listener("script-source.asset-unloaded", [this, kernel](const value& data) { handle_script_source_unloaded(kernel, data); });
    events.add_listener("script-file.asset-loaded", [this, kernel](const value& data) { handle_script_file_loaded(kernel, data); });
    events.add_listener("script-file.asset-unloaded", [this, kernel](const value& data) { handle_script_file_unloaded(kernel, data); });
  }

  void project_system::tick(driver_kernel* kernel, double dt) {
    PROFILE_SECTION("project_system::tick");
    const auto& config = get_driver().configuration();
    if (get_driver().current_driver_state() == driver_state::DRIVER_STATE_RUNNING &&
        loaded_project->is_empty() && config.project_file.has_value()) {
      filepath project_file = *config.project_file;
      load_project(kernel, project_file);
    }

    if (get_driver().current_driver_state() == driver_state::DRIVER_STATE_RUNNING &&
        loaded_project->is_loading()) {
      const bool csproj_built_and_attached = loaded_project->script_project_mounted();
      const bool scene_graph_loaded = loaded_project->scene_graph_loaded();
      const bool finished_loading = csproj_built_and_attached && scene_graph_loaded;
      if (finished_loading) {
        loaded_project->set_state(project::state::LOADED);
        get_driver().trigger_event("project.loaded");
      } else if (scene_graph_loaded && loaded_project->did_script_project_error_occurred()) {
        CORE_LOG_ERROR("Failed to load project due to script project load error. Unloading project.");
        loaded_project->set_state(project::state::LOAD_FAILED);
        loaded_project->fail_load();
      }
    }

    if (get_driver().current_driver_state() == driver_state::DRIVER_STATE_SHUTTING_DOWN &&
        loaded_project->is_unloading()) {
      const bool script_unload_complete = loaded_project->script_project_unmounted();
      const bool scene_unload_complete = loaded_project->scene_graph_unloaded();
      CORE_LOG_TRACE("[PROJECT] Script unload complete: {}", script_unload_complete);
      CORE_LOG_TRACE("[PROJECT] Scene unload complete: {}", scene_unload_complete);
      if (script_unload_complete && scene_unload_complete) {
        loaded_project->set_state(project::state::EMPTY);
        get_driver().trigger_event("project.unloaded");
      }
    }
  }

  void project_system::shutdown(driver_kernel* kernel) {
    PROFILE_SECTION("project_system::shutdown");
    OTHER_ASSERT(loaded_project != nullptr, "No project loaded in project system shutdown.");
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

    PROFILE_SECTION("project_system::load_project");
    if (loaded_project->is_loaded()) {
      CORE_LOG_ERROR("Can not load project file '{}' while other project is open.", project_file.string());
      return;
    }

    CORE_LOG_DEBUG("Begin project load: {}", project_file.string());
    loaded_project->load_from_file(kernel, project_file);
    last_loaded_project_file = project_file;

    if (kernel->has_core_system<scene_system>()) {
      auto& scenes = kernel->get_core_system<scene_system>();
      scenes.load_project_scene_graph(*loaded_project);
    }
  }

  void project_system::unload_project(driver_kernel* kernel) {
    OTHER_ASSERT(kernel != nullptr, "Kernel pointer is null in project system unload_project.");
    OTHER_ASSERT(loaded_project != nullptr, "No project loaded in project system.");
    PROFILE_SECTION("project_system::unload_project");

    if (!loaded_project->is_loaded()) {
      return;
    }

    loaded_project->unload();
    // have to do this here because need access to scene system
    if (kernel->has_core_system<scene_system>()) {
      OTHER_ASSERT(kernel->has_core_system<asset_system>(), "Asset system is not available in the kernel.");
      auto& assets = kernel->get_core_system<asset_system>();
      auto& scenes = kernel->get_core_system<scene_system>();
      auto& project_scene_graph = scenes.get_scene_graph();
      for (auto& data : loaded_project->get_scenes()) {
        natural_t id = data.scene_id;
        scene* s = project_scene_graph.find_scene(id);
        OTHER_ASSERT(s != nullptr, "Scene with ID '{}' not found in project scene graph.", id);

        assets.begin_asset_unload(s->asset_id);
      }
    }
  }

  bool project_system::is_project_empty() const {
    OTHER_ASSERT(loaded_project != nullptr, "No project loaded in project system.");
    return loaded_project->is_empty();
  }

  bool project_system::is_project_loading() const {
    OTHER_ASSERT(loaded_project != nullptr, "No project loaded in project system.");
    return loaded_project->is_loading();
  }

  bool project_system::is_project_unloading() const {
    OTHER_ASSERT(loaded_project != nullptr, "No project loaded in project system.");
    return loaded_project->is_unloading();
  }

  bool project_system::is_project_loaded() const {
    OTHER_ASSERT(loaded_project != nullptr, "No project loaded in project system.");
    return loaded_project->is_loaded();
  }

  void project_system::handle_project_event(driver_kernel* kernel, const project_event_data& data) {
    PROFILE_SECTION("project_system::handle_project_event");
    CORE_LOG_DEBUG("Received project event '{}' for project '{}' at path '{}'", data.type, data.project_name, data.project_path);

    std::string type = data.type;
    filepath project_path = data.project_path;
    if (type == "load") {
      if (!std::filesystem::exists(project_path)) {
        CORE_LOG_ERROR("Project file '{}' does not exist for project load event.", project_path.string());
        return;
      }

      if (std::filesystem::is_directory(project_path)) {
        if (project_path.string().ends_with(std::filesystem::path::preferred_separator)) {
          project_path = project_path.parent_path();
        }

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
      } else if (std::filesystem::is_regular_file(project_path)) {
        // project_path is already a regular file, no further action needed
      }

      load_project(kernel, project_path);
    } else if (type == "save") {
      // handle_save_project(kernel, data);
    }
  }

  void project_system::handle_new_project(driver_kernel* kernel, const value& data) {
    get_driver().on_create_project();
  }

  void project_system::handle_open_project(driver_kernel* kernel, const value& data) {
    sibling<rendering_system>(*kernel).show_open_file_dialog(&detail::open_project_callback, &get_driver(), 0);
  }

  void project_system::handle_save_project(driver_kernel* kernel, const value& data) {
  }

  void project_system::handle_script_project_loaded(driver_kernel* kernel, const value& data) {
    OTHER_ASSERT(loaded_project != nullptr, "No project loaded in project system.");
    OTHER_ASSERT(data.type() == value_type::UINT64, "Expected asset ID as uint64 in script project loaded event data.");
    PROFILE_SECTION("project_system::handle_script_project_loaded");

    CORE_LOG_DEBUG("Script project loaded. Project state: {}", loaded_project->get_state());
    if (loaded_project->is_loading()) {
      natural_t asset_id = data;
      opt<filepath> script = sibling<asset_system>(*kernel).get_local_asset_path(asset_id);
      if (!script.has_value()) {
        CORE_LOG_ERROR("Failed to get local asset path for loaded script project asset with ID: {}", asset_id);
        return;
      }
      OTHER_ASSERT(std::filesystem::exists(script.value()), "Local asset path '{}' for loaded script project asset with ID {} does not exist.", script.value().string(), asset_id);
    } else {
      CORE_LOG_WARN("Unimplemented handling of script project asset loaded event in project", loaded_project->get_state());
    }
  }

  void project_system::handle_script_project_load_failed(driver_kernel* kernel, const value& data) {
    OTHER_ASSERT(loaded_project != nullptr, "No project loaded in project system.");
    OTHER_ASSERT(data.type() == value_type::UINT64, "Expected asset ID as uint64 in script project load failed event data.");
    PROFILE_SECTION("project_system::handle_script_project_load_failed");

    CORE_LOG_DEBUG("Script project load failed. Project state: {}", loaded_project->get_state());
    if (loaded_project->is_loading()) {
      natural_t asset_id = data;
      opt<filepath> script = sibling<asset_system>(*kernel).get_local_asset_path(asset_id);
      if (!script.has_value()) {
        CORE_LOG_ERROR("Failed to get local asset path for script project asset with ID: {} that failed to load", asset_id);
        return;
      }

      loaded_project->error_building_script_project();
      CORE_LOG_ERROR("Failed to load script project asset with ID: {} at path '{}'", asset_id, script.value().string());
    } else {
      CORE_LOG_WARN("Unimplemented handling of script project asset load failed event in project for project state {}", loaded_project->get_state());
    }
  }

  void project_system::handle_script_source_loaded(driver_kernel* kernel, const value& data) {
    OTHER_ASSERT(loaded_project != nullptr, "No project loaded in project system.");
    OTHER_ASSERT(data.type() == value_type::UINT64, "Expected asset ID as uint64 in script source loaded event data.");
    PROFILE_SECTION("project_system::handle_script_source_loaded");

    natural_t asset_id = data;
    opt<filepath> script_source_path = sibling<asset_system>(*kernel).get_local_asset_path(asset_id);

    if (!script_source_path.has_value()) {
      CORE_LOG_ERROR("Failed to get local asset path for loaded script source asset with ID: {}", asset_id);
      return;
    }
    OTHER_ASSERT(std::filesystem::exists(script_source_path.value()), "Local asset path '{}' for loaded script source asset with ID {} does not exist.", script_source_path.value().string(), asset_id);
    CORE_LOG_DEBUG("Script source loaded with path '{}' for asset ID {}", script_source_path.value().string(), asset_id);

    CORE_LOG_DEBUG("Script source loaded. Project State: {}", loaded_project->get_state());
    if (loaded_project->is_loading()) {
      loaded_project->add_built_script(script_source_path.value());
    } else if (loaded_project->is_loaded()) {
      /// hot reload: the plan just refreshed the project assembly artifact
      if (loaded_project->refresh_built_script(script_source_path.value())) {
        get_driver().trigger_event("project.assembly-refreshed");  /// instance-refresh consumers
      }
    } else {
      CORE_LOG_WARN("Unimplemented handling of script source asset loaded event in project for project state {}", loaded_project->get_state());
    }
  }

  void project_system::handle_script_source_load_failed(driver_kernel* kernel, const value& data) {
    OTHER_ASSERT(loaded_project != nullptr, "No project loaded in project system.");
    OTHER_ASSERT(data.type() == value_type::UINT64, "Expected asset ID as uint64 in script source load failed event data.");
    PROFILE_SECTION("project_system::handle_script_source_load_failed");
    natural_t asset_id = data;
    CORE_LOG_ERROR("Failed to load script source asset with ID: {}", asset_id);
  }

  void project_system::handle_script_source_unloaded(driver_kernel* kernel, const value& data) {
    OTHER_ASSERT(loaded_project != nullptr, "No project loaded in project system.");
    OTHER_ASSERT(data.type() == value_type::UINT64, "Expected asset ID as uint64 in script source unloaded event data.");
    PROFILE_SECTION("project_system::handle_script_source_unloaded");

    natural_t asset_id = data;
    opt<filepath> script_source_path = sibling<asset_system>(*kernel).get_local_asset_path(asset_id);

    if (!script_source_path.has_value()) {
      CORE_LOG_ERROR("Failed to get local asset path for unloaded script source asset with ID: {}", asset_id);
      return;
    }
    OTHER_ASSERT(std::filesystem::exists(script_source_path.value()), "Local asset path '{}' for unloaded script source asset with ID {} does not exist.", script_source_path.value().string(), asset_id);
    CORE_LOG_DEBUG("Script source unloaded with path '{}' for asset ID {}", script_source_path.value().string(), asset_id);

    CORE_LOG_DEBUG("Script source unloaded. Project State: {}", loaded_project->get_state());
    if (loaded_project->is_unloading()) {
      loaded_project->remove_built_script(script_source_path.value());
    } else if (loaded_project->is_loaded()) {
      loaded_project->begin_assembly_refresh(script_source_path.value());  /// unload half of a refresh
    } else {
      CORE_LOG_WARN("Unimplemented handling of script source asset unloaded event in project for project state {}", loaded_project->get_state());
    }
  }

  void project_system::handle_script_file_loaded(driver_kernel* kernel, const value& data) {
    OTHER_ASSERT(loaded_project != nullptr, "No project loaded in project system.");
    OTHER_ASSERT(data.type() == value_type::UINT64, "Expected asset ID as uint64 in script file loaded event data.");
    PROFILE_SECTION("project_system::handle_script_file_loaded");

    natural_t asset_id = data;
    opt<filepath> script_file_path = sibling<asset_system>(*kernel).get_local_asset_path(asset_id);

    if (!script_file_path.has_value()) {
      CORE_LOG_ERROR("Failed to get local asset path for loaded script file asset with ID: {}", asset_id);
      return;
    }
    OTHER_ASSERT(std::filesystem::exists(script_file_path.value()), "Local asset path '{}' for loaded script file asset with ID {} does not exist.", script_file_path.value().string(), asset_id);
    CORE_LOG_DEBUG("Script file loaded with path '{}' for asset ID {}", script_file_path.value().string(), asset_id);

    loaded_project->add_script_file(script_file_path.value());
  }

  void project_system::handle_script_file_unloaded(driver_kernel* kernel, const value& data) {
    OTHER_ASSERT(loaded_project != nullptr, "No project loaded in project system.");
    OTHER_ASSERT(data.type() == value_type::UINT64, "Script file unloaded event data must be of type UINT64.");
    PROFILE_SECTION("project_system::handle_script_file_unloaded");

    natural_t asset_id = data;
    auto script_file_path = sibling<asset_system>(*kernel).get_local_asset_path(asset_id);
    if (!script_file_path.has_value()) {
      CORE_LOG_ERROR("Failed to get local asset path for unloaded script file asset with ID: {}", asset_id);
      return;
    }
    OTHER_ASSERT(std::filesystem::exists(script_file_path.value()), "Local asset path '{}' for unloaded script file asset with ID {} does not exist.", script_file_path.value().string(), asset_id);
    CORE_LOG_DEBUG("Script file unloaded with path '{}' for asset ID {}", script_file_path.value().string(), asset_id);

    loaded_project->remove_script_file(script_file_path.value());
  }

  void project_system::load_plugin(const std::string& plugin_name, const filepath& plugin_path) {
    OTHER_ASSERT(std::filesystem::exists(plugin_path), "Plugin file '{}' does not exist.", plugin_path.string());
    OTHER_ASSERT(std::filesystem::is_regular_file(plugin_path), "Plugin file '{}' is not a regular file.", plugin_path.string());
    // clang-format off
    OTHER_ASSERT(plugin_path.extension() == ".dll" || plugin_path.extension() == ".so" || plugin_path.extension() == ".dylib", 
                 "Plugin file '{}' does not have a valid dynamic library extension.", plugin_path.string());
    // clang-format on
    PROFILE_SECTION("project_system::load_plugin");

    CORE_LOG_DEBUG("Loading project plugin: '{}' @ {}", plugin_name, plugin_path.string());
    auto* lib = plugin::load_plugin_library(plugin_path.string());
    if (lib == nullptr) {
      CORE_LOG_ERROR("Failed to load project plugin library: {}", plugin_path.string());
      return;
    }

    auto& driver_kernel = get_driver().get_kernel();
    driver_kernel.register_project_plugin(plugin_path, lib);
  }

  void project_system::unload_plugins() {
    get_driver().get_kernel().unload_project_plugins();
  }

}  // namespace other
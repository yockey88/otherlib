/**
 * \file driver/driver.cpp
 **/
#include "driver/driver.hpp"

#include <sstream>

#include <SDL3/SDL.h>
#include <SDL3/SDL_events.h>

#include "core/frame_rate_enforcer.hpp"
#include "core/logger_sinks.hpp"
#include "thread/thread_safety.hpp"

#include "network/tcp/tcp_transport_provider.hpp"

#include "driver/systems/network_system.hpp"
#include "driver/systems/project_system.hpp"
#include "driver/systems/scene_system.hpp"
#include "driver/systems/scripting_system.hpp"
#include "scripting/interfaces/networking_interfaces.hpp"
#include "scripting/interfaces/rendering_interfaces.hpp"
#include "tools/environment_console_sink.hpp"
#include "tools/scene_cli_tool.hpp"

namespace other {

  void bind_otherlib_driver_lua_functions(lua_host& lua_host, driver* host_driver);

  driver::driver(const command_line& cmd, const config_table& config)
      : config(config), cmd_line(cmd) {
  }

  driver::~driver() {
  }

  void driver::initialize(const command_line& cmd, const subsystem_registry& registry) {
    PROFILE_SECTION("driver::initialize");

    if (cmd_line.project_file.has_value()) {
      if (config.project_file.has_value()) {
        CORE_LOG_WARN("Overwriting project specified in config '{}' with command line project '{}'", config.project_file.value().string(), cmd_line.project_file.value().string());
      }

      config.project_file = cmd_line.project_file;
    }

    state_machine.handle_event(driver_event::DRIVER_EVENT_START, this);
    driver_metadata = build_metadata();

    interfaces.register_interface(get_server_interface());
    interfaces.register_interface(get_ui_window_interface());

    driver_kernel_ptr = make_scope<driver_kernel>(this);
    driver_kernel_ptr->load_profile(registry.get_current_profile());
    on_system_initialization();

    driver_kernel_ptr->initialize();

    {
      logger* log = subsystem<logger>::get();
      log->create_logger("other-editor-log", spdlog::level::trace);
      log_sink console_log_sink = {
        .id = logger::get_next_sink_id(),
        .sink_name = "console-sink",
        .sink_pattern = "[%l] %v",
        .level = spdlog::level::info,
        .sink_factory = [&](const config_table& config) -> spdlog::sink_ptr {
          return std::make_shared<console_sink_mt>(get_event_system());
        }
      };

      std::string loggers[] = { "other-editor-log", "other-core-log" };
      log->register_sink(loggers, &console_log_sink);
    }

    get_event_system()->register_event("ls.driver-systems");
    get_event_system()->add_listener("ls.driver-systems", [this](const value& data) {
      CORE_LOG_INFO("Driver Systems:\n{}", driver_kernel_ptr->list_systems());
    });

    if (driver_kernel_ptr->has_core_system<project_system>()) {
      get_event_system()->add_listener("project.loaded", [this](const value& data) {
        on_project_loaded();
      });
      get_event_system()->add_listener("project.unloaded", [this](const value& data) {
        on_project_unloaded();
      });
    }

    if (!subsystem<scripting_environment>::inert) {
      auto* env = subsystem<scripting_environment>::get();
      OTHER_ASSERT(env != nullptr, "scripting_environment null in load_client!");

      do_script_interface_bindings(this);
      /// lua gets special treatment
      bind_otherlib_driver_lua_functions(env->get_lua_host(), this);
    }

    if (!driver_kernel_ptr->get_core_system<network_system>().network_active()) {
      confirm_initialization();
    }
  }

  void driver::run() {
    ASSERT_MAIN_THREAD();
    PROFILE_SECTION("driver::main_loop");

    CORE_LOG_DEBUG("Entering main driver loop");

    frame_rate_enforcer<120> frame_rate_guard;
    do {
      auto* falloc = arena::get_frame_allocator();
      OTHER_ASSERT(falloc != nullptr, "Failed to get frame allocator at start of main loop.");
      falloc->reset();

      MARK_NAMED_FRAME("driver_main_loop");
      update();
      render();
      frame_rate_guard.wait();
    } while (current_driver_state() != driver_state::DRIVER_STATE_STOPPED);
  }

  void driver::shutdown() {
    ASSERT_MAIN_THREAD();
    PROFILE_SECTION("driver::shutdown");

    {
      PROFILE_SECTION("driver::shutdown--client-on_shutdown");
      on_shutdown();
    }

    driver_kernel_ptr->shutdown();
    driver_kernel_ptr->unload_plugins();
  }

  std::pair<driver*, std::string> driver::create(const command_line& cmd, const config_table& config) {
    PROFILE_SECTION("driver::create");
    driver* driver_instance = nullptr;

    std::string driver_path = config.dynamic_driver_rel_path.value_or("");
    std::string driver_name = "";

    /// if no path then run built-in driver/event loop with environment terminal
    if (driver_path.empty()) {
      CORE_LOG_DEBUG("Creating static driver instance");
      driver_name = config.get_value<std::string>("application.name", "static-driver");
      return { ::otherlib_create_driver(&cmd, &config), driver_name };
    }
    /// otherwise attempt to load the driver and run it
    else {
      CORE_LOG_INFO("Attempting to load dynamic driver from path: {}", driver_path);

      library_handle* lib_handle = plugin::load_plugin_library(driver_path);
      if (lib_handle == nullptr) {
        CORE_LOG_ERROR("Failed to load plugin library: {}", driver_path);
        return { nullptr, "" };
      }
      driver_name = filepath(driver_path).filename().stem().string();
      CORE_LOG_DEBUG("Loaded plugin library [{}] : {}", driver_name, driver_path);

      auto sym_res = lib_handle->get_symbol(driver::kDynamicDriverFactorySymbolName);
      if (!sym_res.has_value()) {
        CORE_LOG_ERROR("Failed to get symbol '{}' from plugin '{}'", driver::kDynamicDriverFactorySymbolName, driver_path);
        return { nullptr, "" };
      }

      symbol& sym = sym_res.value();
      if (sym.address == nullptr) {
        CORE_LOG_ERROR("Failed to load symbol '{}' from plugin '{}'", driver::kDynamicDriverFactorySymbolName, driver_path);
        return { nullptr, "" };
      }

      CORE_LOG_DEBUG("calling '{}' from plugin [{}]", driver::kDynamicDriverFactorySymbolName, driver_name);
      driver* (*fn)(const command_line*, const config_table*) = sym.get_function<driver* (*)(const command_line*, const config_table*)>();
      driver_instance = fn(&cmd, &config);
      CORE_LOG_DEBUG("Loaded driver [{}]", driver_name);

      if (driver_instance == nullptr) {
        CORE_LOG_ERROR("Failed to create driver instance from plugin '{}'", driver_path);
        plugin::unload_plugin_library(driver_name);
        return { nullptr, "" };
      }
    }

    driver_instance->dynamic = true;
    return { driver_instance, driver_name };
  }

  void driver::destroy(const std::string& name, driver* instance) {
    OTHER_ASSERT(instance != nullptr, "Cannot destroy a null driver instance.");
    PROFILE_SECTION("driver::destroy");

    if (!instance->dynamic) {
      CORE_LOG_DEBUG("Destroying driver instance.");
      ::otherlib_destroy_driver(instance);
      return;
    }

    library_handle* lib_handle = plugin::get_plugin_library(name);
    OTHER_ASSERT(lib_handle != nullptr, "Failed to get plugin library: {}", name);

    auto sym_res = lib_handle->get_symbol(driver::kDynamicDriverDestroySymbolName);
    OTHER_ASSERT(sym_res.has_value(), "Failed to get symbol '{}' from plugin '{}'", driver::kDynamicDriverDestroySymbolName, name);

    symbol& sym = sym_res.value();
    OTHER_ASSERT(sym.address != nullptr, "Failed to load symbol '{}' from plugin '{}'", driver::kDynamicDriverDestroySymbolName, name);

    CORE_LOG_DEBUG("calling '{}' from plugin [{}]", driver::kDynamicDriverDestroySymbolName, name);
    sym.get_function<void (*)(driver*)>()(instance);
    plugin::unload_plugin_library(name);
  }

  natural_t driver::begin_asset_load(const filepath& asset_path) {
    OTHER_ASSERT(driver_kernel_ptr != nullptr, "Driver kernel is not initialized.");
    return driver_kernel_ptr->get_core_system<asset_system>().begin_asset_load(asset_path);
  }

  void driver::begin_asset_unload(natural_t asset_id) {
    OTHER_ASSERT(driver_kernel_ptr != nullptr, "Driver kernel is not initialized.");
    driver_kernel_ptr->get_core_system<asset_system>().begin_asset_unload(asset_id);
  }

  natural_t driver::add_model_source_asset(const std::string& name, const std::span<const vertex> vertices, const std::span<const index> indices) {
    OTHER_ASSERT(driver_kernel_ptr != nullptr, "Driver kernel is not initialized.");
    return driver_kernel_ptr->get_core_system<asset_system>().add_model_source_asset(name, vertices, indices);
  }

  natural_t driver::add_scene_asset(scene* scene_ptr, opt<filepath> scene_path) {
    OTHER_ASSERT(driver_kernel_ptr != nullptr, "Driver kernel is not initialized.");
    return driver_kernel_ptr->get_core_system<asset_system>().add_scene_asset(scene_ptr, scene_path);
  }

  natural_t driver::add_rendering_pipeline_asset(const std::string_view name, const pipeline_definition& definition) {
    OTHER_ASSERT(driver_kernel_ptr != nullptr, "Driver kernel is not initialized.");
    return driver_kernel_ptr->get_core_system<asset_system>().add_rendering_pipeline_asset(name, definition);
  }

  natural_t driver::get_asset_hash(natural_t asset_id) const {
    OTHER_ASSERT(driver_kernel_ptr != nullptr, "Driver kernel is not initialized.");
    return driver_kernel_ptr->get_core_system<asset_system>().get_asset_hash(asset_id);
  }

  natural_t driver::get_asset_state(natural_t asset_id) const {
    OTHER_ASSERT(driver_kernel_ptr != nullptr, "Driver kernel is not initialized.");
    return driver_kernel_ptr->get_core_system<asset_system>().get_asset_state(asset_id);
  }

  natural_t driver::get_asset_id_from_path(const filepath& path) const {
    OTHER_ASSERT(driver_kernel_ptr != nullptr, "Driver kernel is not initialized.");
    return driver_kernel_ptr->get_core_system<asset_system>().get_asset_id_from_path(path);
  }

  void driver::process_driver_event(driver_event event) {
    state_machine.handle_event(event, this);
  }

  void driver::request_shutdown() {
    if (current_driver_state() == driver_state::DRIVER_STATE_SHUTTING_DOWN ||
        current_driver_state() == driver_state::DRIVER_STATE_STOPPED) {
      return;
    }

    /// check if project is loading or unloading, of so, we put the shutdown request off
    //  until project is finished loading or unloading.
    if (driver_kernel_ptr->has_core_system<project_system>()) {
      auto& projects = driver_kernel_ptr->get_core_system<project_system>();
      CORE_LOG_DEBUG("Project state on shutdown request: {}", projects.get_project().get_state());
      if (projects.is_project_loading() || projects.is_project_unloading()) {
        CORE_LOG_INFO("Project is currently loading or unloading, deferring shutdown request.");
        runtime_state.shutdown_requested = true;
        return;
      }
    }

    begin_shutdown_sequence();
  }

  std::string driver::get_driver_info_string(const std::string_view str) const {
    CORE_LOG_DEBUG("object-driver-info argument: {}", str);
    std::stringstream ss;
    switch (FNV(str)) {
      default:
        ss << "Unknown driver info argument: '" << str << "'";
        break;
    }
    return ss.str();
  }

  void driver::confirm_initialization() {
    OTHER_ASSERT(driver_kernel_ptr != nullptr, "Driver kernel is not initialized.");
    PROFILE_SECTION("driver::confirm_initialization");
    CORE_LOG_DEBUG("Confirming initialization...");

    /// cli tools reachable in-process via other::cli::run; drivers only launch from
    ///  source trees, so dev workflow tools ride along here
    cli::register_dev_tools(cli::default_tool_registry());
    cli::register_environment_tools(cli::default_tool_registry());

    {
      PROFILE_SECTION("driver::confirm_initialization--on_initialization_confirm");
      on_initialization_confirm();
    }

    if (network_enabled()) {
      OTHER_ASSERT(driver_kernel_ptr->has_core_system<network_system>(), "Network system is not initialized in driver kernel.");
      auto& net_system = driver_kernel_ptr->get_core_system<network_system>();
      net_system.register_transport_provider(make_scope<tcp_transport_provider>());
      // net_system.register_transport_provider(make_scope<udp_transport_provider>());
      // net_system.register_transport_provider(make_scope<loopback_transport_provider>());
    }

    {
      PROFILE_SECTION("driver::initialize--on_early_initialize");
      on_early_initialize();
    }

    driver_kernel_ptr->load_driver_plugins_from_config(this);
    load_client();

    driver_kernel_ptr->driver_initialized();

    {
      PROFILE_SECTION("driver::confirm_initialization--on_initialize");
      on_initialize();
    }

    process_driver_event(driver_event::DRIVER_EVENT_READY);
  }

  void driver::confirm_shutdown() {
    OTHER_ASSERT(driver_kernel_ptr != nullptr, "Driver kernel is not initialized.");
    PROFILE_SECTION("driver::confirm_shutdown");
    CORE_LOG_DEBUG("Confirming shutdown...");

    on_shutdown_confirm();

    driver_kernel_ptr->unload_driver_plugins();
    process_driver_event(driver_event::DRIVER_EVENT_READY);
  }

  void driver::confirm_assets_clean() {
    CORE_LOG_DEBUG("Asset system shutdown confirmed.");
    shutdown_state.asset_manager_shutdown = true;
  }

  void driver::confirm_network_thread_shutdown() {
    CORE_LOG_DEBUG("Network thread shutdown confirmed.");
    shutdown_state.network_thread_shutdown = true;
  }

  void driver::trigger_event(const std::string& event_name, const value& data) {
    OTHER_ASSERT(driver_kernel_ptr != nullptr, "Driver kernel is not initialized.");
    driver_kernel_ptr->get_core_system<event_driver_system>().trigger_event(driver_kernel_ptr.get(), event_name, data);
  }

  std::string driver::get_project_name() const {
    return get_metadata().name;
  }

  std::string driver::get_project_description() const {
    return get_metadata().description;
  }

  std::string driver::get_project_author() const {
    return get_metadata().author;
  }

  std::string driver::get_project_version() const {
    return get_metadata().version;
  }

  bool driver::should_auto_play_scenes() const {
    return get_config_value<bool>("application.auto-play-loaded-scenes", true);
  }

  bool driver::project_loaded() const {
    OTHER_ASSERT(driver_kernel_ptr != nullptr, "Driver kernel is not initialized.");
    return driver_kernel_ptr->get_core_system<project_system>().project_loaded();
  }

  scene* driver::get_active_scene() {
    OTHER_ASSERT(driver_kernel_ptr != nullptr, "Driver kernel is not initialized.");
    return driver_kernel_ptr->get_core_system<scene_system>().get_active_scene();
  }

  renderer& driver::get_renderer() {
    OTHER_ASSERT(rendering_enabled(), "Rendering is not enabled, cannot get renderer.");
    OTHER_ASSERT(driver_kernel_ptr != nullptr, "Driver kernel is not initialized.");
    auto& r = driver_kernel_ptr->get_core_system<rendering_system>().get_renderer();
    OTHER_ASSERT(r != nullptr, "Renderer is not initialized.");
    return *r;
  }

  asset* driver::get_asset(natural_t asset_id) {
    OTHER_ASSERT(driver_kernel_ptr != nullptr, "Driver kernel is not initialized.");
    return driver_kernel_ptr->get_core_system<asset_system>().get_asset(asset_id);
  }

  void driver::handle_rendering_pipeline_loaded(natural_t asset_id, render_pipeline* pipeline) {
    OTHER_ASSERT(asset_id != 0, "Invalid asset ID for loaded rendering pipeline.");
    OTHER_ASSERT(pipeline != nullptr, "Loaded rendering pipeline is null.");

    CORE_LOG_DEBUG("Rendering pipeline loaded: asset_id={}, pipeline_name={}", asset_id, pipeline->get_definition().name);
    on_rendering_pipeline_loaded(asset_id, pipeline);
  }

  void driver::handle_rendering_pipeline_unloaded(natural_t asset_id, render_pipeline* pipeline) {
    OTHER_ASSERT(asset_id != 0, "Invalid asset ID for unloaded rendering pipeline.");
    OTHER_ASSERT(pipeline != nullptr, "Unloaded rendering pipeline is null.");

    CORE_LOG_DEBUG("Rendering pipeline unloaded: asset_id={}, pipeline_name={}", asset_id, pipeline->get_definition().name);
    on_rendering_pipeline_unloaded(asset_id, pipeline);
  }

  void driver::input_event(const input_state_change_event& event) {
    on_input_event(event);
  }

  void driver::file_event(const struct file_event& event) {
    on_file_event(event);

    switch (event.type) {
      case file_event::type::CREATED:
      case file_event::type::MODIFIED:
      case file_event::type::DELETED:
        handle_file_refresh(event.path);
        break;

      case file_event::type::RENAMED:
        handle_file_refresh(*event.old_path);
        handle_file_refresh(event.path);
        break;

      default:
        CORE_LOG_WARN("Unknown file event type unhandled!");
        break;
    }
  }

  natural_t driver::add_interface(const std::string_view interface_name, sol::table inteface_table) {
    PROFILE_SECTION("driver::add_interface");
    return interfaces.register_interface_binding(interface_name, std::move(inteface_table));
  }

  void driver::handle_file_refresh(const filepath& path) {
    PROFILE_SECTION("driver::handle_file_refresh");
    CORE_LOG_DEBUG("File refresh event for path: {}", path.string());

    if (!driver_kernel_ptr->has_core_system<asset_system>()) {
      CORE_LOG_WARN("Asset system is not initialized, cannot handle file refresh for path: {}", path.string());
      return;
    }

    auto& assets = driver_kernel_ptr->get_core_system<asset_system>();
    natural_t asset_id = assets.get_asset_id_from_path(path);
    if (asset_id != 0) {
      assets.reload_asset(asset_id);
    } else {
      assets.file_changed(path);
    }
  }

  driver::metadata driver::build_metadata() {
    PROFILE_SECTION("driver::build_metadata");
    const auto md = configuration().get_raw("application.metadata");

    metadata data = {};
    if (md) {
      if (md.is_array_of_tables()) {
        const auto* metadata_tables = md.as_array();
        OTHER_ASSERT(metadata_tables != nullptr, "Invalid format for application metadata in configuration. Expected an array of tables.");

        for (const auto& table : *metadata_tables) {
          OTHER_ASSERT(table.is_table(), "Invalid format for application metadata in configuration. Expected an array of tables.");
          const auto* metadata_table = table.as_table();
          OTHER_ASSERT(metadata_table != nullptr, "Invalid format for application metadata in configuration. Expected an array of tables.");

          auto key_itr = metadata_table->find("key");
          auto value_itr = metadata_table->find("value");
          if (key_itr == metadata_table->end() || value_itr == metadata_table->end()) {
            CORE_LOG_ERROR("Invalid format for application metadata entry in configuration. Each metadata entry must contain 'key' and 'value' fields.");
            continue;
          }

          if (!key_itr->second.is_string()) {
            CORE_LOG_ERROR("Invalid format for application metadata key in configuration. 'key' field must be a string.");
            continue;
          }

          auto* key = key_itr->second.as_string();
          OTHER_ASSERT(key != nullptr, "Invalid format for application metadata key in configuration. 'key' field must be a string.");

          std::string k = key->get();
          CORE_LOG_DEBUG("Processing application metadata entry with key '{}'", k);
          /// \todo fix this
          if (k == "name") {
            if (!value_itr->second.is_string()) {
              CORE_LOG_ERROR("Invalid format for application metadata value in configuration. 'value' field for 'name' key must be a string.");
              continue;
            }
            data.name = value_itr->second.as_string()->get();
          } else if (k == "description") {
            if (!value_itr->second.is_string()) {
              CORE_LOG_ERROR("Invalid format for application metadata value in configuration. 'value' field for 'description' key must be a string.");
              continue;
            }
            data.description = value_itr->second.as_string()->get();
          } else if (k == "author") {
            if (!value_itr->second.is_string()) {
              CORE_LOG_ERROR("Invalid format for application metadata value in configuration. 'value' field for 'author' key must be a string.");
              continue;
            }
            data.author = value_itr->second.as_string()->get();
          } else if (k == "version") {
            if (!value_itr->second.is_string()) {
              CORE_LOG_ERROR("Invalid format for application metadata value in configuration. 'value' field for 'version' key must be a string.");
              continue;
            }
            data.version = value_itr->second.as_string()->get();
          } else {
            CORE_LOG_WARN("Unknown application metadata key '{}' in configuration. This key will be ignored.", k);
          }
        }
      } else {
        CORE_LOG_ERROR("Invalid format for application metadata in configuration. Expected an array of tables.");
      }
    }

    // clang-format off
    CORE_LOG_DEBUG("\nFinished processing application metadata from configuration.\nResult: name='{}', description='{}', author='{}', version='{}'\n", 
                    data.name, data.description, data.author, data.version);
    // clang-format on

    return data;
  }

  void driver::load_client() {
    if (!subsystem<scripting_environment>::inert) {
      PROFILE_SECTION("driver::initialize--client-run-envrc");

      auto* env = subsystem<scripting_environment>::get();
      OTHER_ASSERT(env != nullptr, "scripting_environment null in load_client!");

      if (std::string envrc_path = get_config_value<std::string>("scripting.init-lua"); !envrc_path.empty() && std::filesystem::exists(envrc_path)) {
        /// this one has to be loaded into the host without the sandboxing of the environment
        ///  as this is supposed to be the user's customization of the environment
        auto& lua_host = env->get_lua_host();
        sol::state& lua_state = lua_host.get_lua_state();

        try {
          lua_state.script_file(envrc_path);
        } catch (const sol::error& e) {
          CORE_LOG_ERROR("Failed to run driver environment runtime script: {}\nLua Error: {}", envrc_path, e.what());
        } catch (...) {
          CORE_LOG_ERROR("Failed to run driver environment runtime script: {}", envrc_path);
        }
      }
    }
  }

  filepath driver::get_project_cache() {
    filepath cache_file = get_app_data_folder("OtherEngine/OtherServer") / filepath("project_cache.json");
    if (!std::filesystem::exists(cache_file)) {
      std::ofstream file(cache_file);
      file << "{}";
      file.close();
    }
    return cache_file;
  }

  void driver::update() {
    ASSERT_MAIN_THREAD();
    PROFILE_SECTION("driver::update");
    double dt = frame_delta_time;

    driver_kernel_ptr->tick(dt);

    {
      PROFILE_SECTION("driver::update--on_update");
      on_update();
    }
    switch (current_driver_state()) {
      case driver_state::DRIVER_STATE_INITIALIZING: update_initializing(); break;

      case driver_state::DRIVER_STATE_RUNNING: {
        const bool should_lock = runtime_state.queued_project_file.has_value();
        if (should_lock) {
          std::lock_guard lock(runtime_state.mutex);
          driver_kernel_ptr->get_core_system<project_system>().load_project(driver_kernel_ptr.get(), runtime_state.queued_project_file.value());
          runtime_state.queued_project_file = std::nullopt;
        }

        {
          PROFILE_SECTION("driver::update--update_running");
          update_running();
        }
      } break;

      case driver_state::DRIVER_STATE_SHUTTING_DOWN: {
        update_shutting_down();

        if (shutdown_state.ready_to_shutdown(this)) {
          confirm_shutdown();
        }
      } break;

      case driver_state::DRIVER_STATE_STOPPED: break;
      default:
        OTHER_ASSERT(false, "Driver in unknown state {}", current_driver_state());
        break;
    }

    subsystem<input_system>::get()->finalize_frame();
  }

  void driver::render() {
    ASSERT_MAIN_THREAD();
    if (!rendering_enabled()) {
      return;
    }
    PROFILE_SECTION("driver::render");
    {
      PROFILE_SECTION("driver::render--on_render");
      on_render();
    }

    if (driver_kernel_ptr->has_core_system<rendering_system>()) {
      driver_kernel_ptr->get_core_system<rendering_system>().render(driver_kernel_ptr.get());
    }
  }

  void driver::on_project_loaded() {
    ASSERT_MAIN_THREAD();
    OTHER_ASSERT(driver_kernel_ptr != nullptr, "Driver kernel is not initialized.");
    PROFILE_SECTION("driver::on_project_loaded");
    auto& p = driver_kernel_ptr->get_core_system<project_system>().get_project();
    OTHER_ASSERT(p.is_loaded(), "Project is not loaded in on_project_loaded!");
    CORE_LOG_DEBUG("Project loaded: {}", p.get_project_name());

    if (driver_kernel_ptr->has_core_system<scene_system>()) {
      auto& scenes = driver_kernel_ptr->get_core_system<scene_system>();

      /// do this before running rc file in case rc file loads a scene
      if (auto* curr_scene = get_active_scene(); curr_scene != nullptr) {
        OTHER_ASSERT(false, "TODO: implement the case where a scene was opened before a project or independently of it's parent project");
      }

      natural_t starting_scene_id = p.get_starting_scene_id();
      if (starting_scene_id != 0) {
        CORE_LOG_DEBUG("Project starting scene ID: {}", starting_scene_id);
        scenes.set_scene_to_active(starting_scene_id);
      }
    }

    filepath rc_path = p.get_project_rc_path();
    if (!rc_path.empty() && std::filesystem::exists(rc_path)) {
      PROFILE_SECTION("driver::on_project_loaded--run-project-rc");
      /// run driver envrc file if it exists
      auto* env = subsystem<scripting_environment>::get();
      OTHER_ASSERT(env != nullptr, "scripting_environment null in on_project_loaded!");

      /// this one has to be loaded into the host without the sandboxing of the environment
      ///  as this is supposed to be the user's customization of the environment
      auto& lua_host = env->get_lua_host();
      sol::state& lua_state = lua_host.get_lua_state();

      try {
        lua_state.script_file(rc_path.string());
      } catch (const sol::error& e) {
        CORE_LOG_ERROR("Failed to run driver environment runtime script: {}\nLua Error: {}", rc_path.string(), e.what());
      } catch (...) {
        CORE_LOG_ERROR("Failed to run driver environment runtime script: {}", rc_path.string());
      }
    }

    if (runtime_state.shutdown_requested) {
      begin_shutdown_sequence();
    }
  }

  void driver::on_project_unloaded() {
    OTHER_ASSERT(driver_kernel_ptr != nullptr, "Driver kernel is not initialized.");
    CORE_LOG_DEBUG("Project unloaded.");
    shutdown_state.project_unloaded = true;

    if (runtime_state.shutdown_requested) {
      begin_shutdown_sequence();
    }
  }

  void driver::launch_detached_process(const filepath& working_dir, const filepath& exe_name, const std::span<const std::string> args) {
    // launch_process(working_dir, exe_name, args);
  }

  void driver::handle_driver_event_with_lua_table(const std::string_view event_name, const sol::table& event_data) {
    if (event_name.starts_with("project.")) {
      std::string project_name = event_data["project_name"].get_or(std::string("unknown"));
      std::string project_path = event_data["project_path"].get_or(std::string("unknown"));

      project_event_data data = {
        .type = std::string(event_name).substr(std::string("project.").size()),
        .project_name = project_name,
        .project_path = project_path,
      };
      trigger_event("native-" + std::string(event_name), data);
    } else {
      CORE_LOG_WARN("Received Lua table event '{}' but no handler is registered for it", event_name);
    }
  }

  void driver::begin_shutdown_sequence() {
    PROFILE_SECTION("driver::begin_shutdown_sequence");
    CORE_LOG_DEBUG("Beginning shutdown sequence");

    if (driver_kernel_ptr->has_core_system<rendering_system>()) {
      auto& rendering = driver_kernel_ptr->get_core_system<rendering_system>();
      rendering.close_all_windows();
    }

    if (driver_kernel_ptr->has_core_system<scene_system>()) {
      auto& scenes = driver_kernel_ptr->get_core_system<scene_system>();
      scenes.unload_active_scene();
    }

    if (driver_kernel_ptr->has_core_system<project_system>()) {
      auto& projects = driver_kernel_ptr->get_core_system<project_system>();
      CORE_LOG_DEBUG("Project state on shutdown request: {}", projects.get_project().get_state());
      if (projects.is_project_loaded()) {
        projects.unload_project(driver_kernel_ptr.get());
      } else if (projects.is_project_empty()) {
        shutdown_state.project_unloaded = true;
      }
    } else {
      /// profiles without a project system have nothing to unload; the shutdown gate
      ///  must not wait on it
      shutdown_state.project_unloaded = true;
    }

    driver_kernel_ptr->get_core_system<network_system>().begin_shutdown_sequence(driver_kernel_ptr.get());
    driver_kernel_ptr->get_core_system<asset_system>().begin_full_unload();

    on_shutdown_request();
    process_driver_event(driver_event::DRIVER_EVENT_STOP);

    if (!driver_kernel_ptr->get_core_system<network_system>().network_active()) {
      shutdown_state.network_thread_shutdown = true;
    }
  }

  void bind_otherlib_driver_lua_functions(lua_host& lua_host, driver* host_driver) {
    PROFILE_SECTION("bind_otherlib_driver_lua_functions");
    sol::state& lua_state = lua_host.get_lua_state();

    sol::table driver_table = lua_state["__other_native"]["__driver"];
    sol::table scene_table = lua_state["__other_native"]["__scene_interface"];
    scene_table.set_function("get_scene_clear_color", &scene_interface::get_scene_clear_color);
    scene_table.set_function("set_scene_clear_color", &scene_interface::set_scene_clear_color);
    scene_table.set_function("get_object_name", &scene_interface::get_object_name);
    scene_table.set_function("set_object_name", &scene_interface::set_object_name);
    scene_table.set_function("add_tag_to_object", &scene_interface::add_tag_to_object);
    scene_table.set_function("remove_tag_from_object", &scene_interface::remove_tag_from_object);
    scene_table.set_function("attach_dotnet_behavior_to_object", &scene_interface::attach_dotnet_behavior_to_object);
    scene_table.set_function("attach_model_to_object", &scene_interface::attach_model_to_object);
    scene_table.set_function("attach_camera_to_object", &scene_interface::attach_camera_to_object);
    scene_table.set_function("attach_point_light_to_object", &scene_interface::attach_point_light_to_object);
    scene_table.set_function("attach_direction_light_to_object", &scene_interface::attach_direction_light_to_object);
    scene_table.set_function("draw_line", &scene_interface::draw_line);
    scene_table.set_function("draw_triangle", &scene_interface::draw_triangle);
    scene_table.set_function("draw_point", &scene_interface::draw_point);

    driver_table["__native_pointer"] = reinterpret_cast<std::uintptr_t>(host_driver);
    driver_table.set_function("trigger_driver_event", [host_driver](const std::string& event, sol::object data) {
      if (data.get_type() == sol::type::table) {
        host_driver->handle_driver_event_with_lua_table(event, data.as<sol::table>());
        return;
      }

      value val;
      switch (data.get_type()) {
        case sol::type::nil: break;
        case sol::type::boolean:
          val = value(data.as<bool>());
          break;
        case sol::type::number:
          val = value(data.as<double>());
          break;
        case sol::type::string:
          val = value(data.as<std::string>());
          break;
        default:
          CORE_LOG_WARN("Unsupported data type for event user data: {}", data.get_type());
          break;
      }
      host_driver->get_kernel().get_core_system<event_driver_system>().trigger_event(&host_driver->get_kernel(), event, val);
    });
    driver_table.set_function("process_driver_event", [host_driver](driver_event event) {
      host_driver->process_driver_event(event);
    });
    driver_table.set_function("add_main_menu_bar_menu_item", [host_driver](const std::string& menu_name, sol::object menu_item_table) {
      auto& rendering_sys = host_driver->get_kernel().get_core_system<rendering_system>();
      auto& driver_ui = rendering_sys.get_driver_ui();

      sol::table t = menu_item_table.as<sol::table>();
      driver_ui->register_main_menu_bar_menu(rendering_sys.build_menu(menu_name, t));
    });
    driver_table.set_function("add_interface", [host_driver](const std::string& interface_name, sol::table interface_table) {
      return host_driver->add_interface(interface_name, interface_table);
    });

    /// now we bind dotnet types into lua types by asking the dotnet types to write their descriptor tables
    auto* scripting_env = subsystem<scripting_environment>::get();
    OTHER_ASSERT(scripting_env != nullptr, "scripting_environment is not initialized.");

    // auto& dotnet_host = scripting_env->get_dotnet_host();
    // type_cache* types = dotnet_host.get_type_cache();
    // OTHER_ASSERT(types != nullptr, "dotnet_host type cache is null.");

    // for (auto& [type_hash, dotnet_type_ptr] : *types) {
    //   sol::table type_table = dotnet_type_ptr.create_lua_descriptor(lua_state);
    //   CORE_LOG_TRACE("Registering .NET type '{}' in Lua .NET type registry", dotnet_type_ptr.full_name());

    //   lua_state["__other_native"]["__dotnet_types"][dotnet_type_ptr.full_name()] = type_table;
    // }
  }

}  // namespace other
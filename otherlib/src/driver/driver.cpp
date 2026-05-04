/**
 * \file driver/driver.cpp
 **/
#include "driver/driver.hpp"

#include <sstream>

#include <SDL3/SDL.h>
#include <SDL3/SDL_events.h>

#include "core/defines.hpp"
#include "core/logger.hpp"

#include "script/scripting_environment.hpp"

#include "driver/driver_tasks.hpp"
#include "scripting/bindings.hpp"
#include "scripting/interfaces/networking_interfaces.hpp"
#include "scripting/scene_interface.hpp"
#include "vm/other_device.hpp"

namespace other {

  void bind_otherlib_driver_lua_functions(lua_host& lua_host, driver* host_driver);

  driver::driver(const command_line& cmd, const config_table& config)
      : config(config), cmd_line(cmd) {
    this->config.project_file = cmd.project_file;
  }

  void driver::initialize(const command_line& cmd, const subsystem_registry& registry) {
    PROFILE_SECTION("driver::initialize");

    state_machine.handle_event(driver_event::DRIVER_EVENT_START, this);
    driver_metadata = build_metadata();

    interfaces.register_interface(get_server_interface());
    interfaces.register_interface(get_http_server_interface());

    driver_kernel_ptr = make_scope<driver_kernel>(this);
    driver_kernel_ptr->load_profile(registry.get_current_profile());
    driver_kernel_ptr->load_plugins_from_config(this);
    driver_kernel_ptr->initialize();

    get_event_system()->register_event("ls.driver-systems");
    get_event_system()->add_listener("ls.driver-systems", [this](const value& data) {
      CORE_LOG_INFO("Driver Systems:\n{}", driver_kernel_ptr->list_systems());
    });

    if (driver_kernel_ptr->has_core_system<project_system>()) {
      get_event_system()->add_listener("project.loaded", [this](const value& data) {
        on_project_loaded();
      });
    }

    if (!subsystem<scripting_environment>::inert) {
      auto* env = subsystem<scripting_environment>::get();
      OTHER_ASSERT(env != nullptr, "scripting_environment null in load_client!");

      do_script_interface_bindings(this);
      /// lua gets special treatment
      bind_otherlib_driver_lua_functions(env->get_lua_host(), this);
    }

    load_client();

    if (!driver_kernel_ptr->get_core_system<network_system>().network_active()) {
      confirm_initialization();
    }
  }

  void driver::run() {
    PROFILE_SECTION("driver::main_loop");

    CORE_LOG_DEBUG("Entering main driver loop");
    do {
      MARK_NAMED_FRAME("driver_main_loop");
      update();
      render();
    } while (current_driver_state() != driver_state::DRIVER_STATE_STOPPED);
  }

  void driver::shutdown() {
    PROFILE_SECTION("driver::shutdown");
    {
      PROFILE_SECTION("driver::shutdown--client-on_shutdown");
      on_shutdown();
    }

    driver_kernel_ptr->shutdown();
  }

  std::pair<driver*, std::string> driver::create(const command_line& cmd, const config_table& config) {
    driver* driver_instance = nullptr;

    std::string driver_path = config.dynamic_driver_rel_path.value_or("");
    std::string driver_name = "";

    /// if no path then run built-in driver/event loop with environment terminal
    if (driver_path.empty()) {
      CORE_LOG_DEBUG("Creating static driver instance");
      driver_name = config.get_value<std::string>("application.name", "static-driver");
      return { create_driver(&cmd, &config), driver_name };
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

      auto sym_res = lib_handle->get_symbol("create_driver");
      if (!sym_res.has_value()) {
        CORE_LOG_ERROR("Failed to get symbol 'create_driver' from plugin '{}'", driver_path);
        return { nullptr, "" };
      }

      symbol& sym = sym_res.value();
      if (sym.address == nullptr) {
        CORE_LOG_ERROR("Failed to load symbol 'create_driver' from plugin '{}'", driver_path);
        return { nullptr, "" };
      }

      CORE_LOG_DEBUG("calling 'create_driver' from plugin [{}]", driver_name);
      driver* (*fn)(const config_table*) = sym.get_function<driver* (*)(const config_table*)>();
      driver_instance = fn(&config);
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

    if (!instance->dynamic) {
      CORE_LOG_DEBUG("Destroying driver instance.");
      destroy_driver(instance);
      return;
    }

    library_handle* lib_handle = plugin::get_plugin_library(name);
    OTHER_ASSERT(lib_handle != nullptr, "Failed to get plugin library: {}", name);

    auto sym_res = lib_handle->get_symbol("destroy_driver");
    OTHER_ASSERT(sym_res.has_value(), "Failed to get symbol 'destroy_driver' from plugin '{}'", name);

    symbol& sym = sym_res.value();
    OTHER_ASSERT(sym.address != nullptr, "Failed to load symbol 'destroy_driver' from plugin '{}'", name);

    CORE_LOG_DEBUG("calling 'destroy_driver' from plugin [{}]", name);
    sym.get_function<void (*)(driver*)>()(instance);
    plugin::unload_plugin_library(name);
  }

  natural_t driver::begin_asset_load(const filepath& asset_path, std::function<void(natural_t)> on_loaded) {
    OTHER_ASSERT(driver_kernel_ptr != nullptr, "Driver kernel is not initialized.");
    return driver_kernel_ptr->get_core_system<asset_system>().begin_asset_load(asset_path, on_loaded);
  }

  natural_t driver::add_model_source_asset(const std::string& name, const std::vector<vertex>& vertices, const std::vector<index>& indices) {
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

  void driver::process_driver_event(driver_event event) {
    state_machine.handle_event(event, this);
  }

  void driver::request_shutdown() {
    if (current_driver_state() == driver_state::DRIVER_STATE_SHUTTING_DOWN ||
        current_driver_state() == driver_state::DRIVER_STATE_STOPPED) {
      return;
    }
    CORE_LOG_INFO("Beginning shutdown sequence");

    if (driver_kernel_ptr->has_core_system<scene_system>()) {
      auto& scenes = driver_kernel_ptr->get_core_system<scene_system>();
      scenes.unload_active_scene();
      scenes.unload_project_scene_graph();
    }

    driver_kernel_ptr->get_core_system<network_system>().begin_shutdown_sequence(driver_kernel_ptr.get());
    driver_kernel_ptr->get_core_system<asset_system>().begin_full_unload();

    on_shutdown_request();
    process_driver_event(driver_event::DRIVER_EVENT_STOP);

    if (!driver_kernel_ptr->get_core_system<network_system>().network_active()) {
      shutdown_state.network_thread_shutdown = true;
    }
  }

  std::string driver::get_driver_info_string(const std::string_view str) const {
    CORE_LOG_DEBUG("object-driver-info argument: {}", str);
    std::stringstream ss;

    if (str == "<stack>") {
      // ss << "  " << context_stack_top << " objects in context stack.\n";
      // if (context_stack_top > 0) {
      //   scene_object* obj = context_stack[context_stack_top - 1];
      //   ss << "Top of Context Stack Object Info:\n";
      //   ss << "  - Name: " << obj->name << "\n";
      //   ss << "  - ID: " << obj->id << "\n";
      // }
    } else {
      ss << "Unknown driver info argument: '" << str << "'";
    }

    return ss.str();
  }

  void driver::confirm_initialization() {
    CORE_LOG_DEBUG("Confirming initialization...");
    on_initialization_confirm();
    process_driver_event(driver_event::DRIVER_EVENT_READY);
  }

  void driver::confirm_assets_clean() {
    CORE_LOG_DEBUG("Confirming assets are clean...");
    shutdown_state.asset_manager_shutdown = true;
  }

  void driver::confirm_network_thread_shutdown() {
    CORE_LOG_INFO("Network thread shutdown confirmed.");
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

  void driver::input_event(const input_state_change_event& event) {
    on_input_event(event);
  }

  void driver::data_received(natural_t id, std::vector<uint8_t> data) {
    // call ReceiveData if http request fails to parse, don't call both
    if (http::is_http_request(data)) {
      opt<http::request> req_opt = http::parse_http_request(data);
      if (req_opt.has_value()) {
        http_request_received(id, *req_opt);
        return;
      }
    }

    interfaces.invoke("Other.Server", "ReceiveData", id, data);
    on_data_received(id, data);
  }

  void driver::new_connection_accepted(natural_t from_connection_id, natural_t connection_id) {
    on_new_connection_accepted(from_connection_id, connection_id);
    interfaces.invoke("Other.Server", "AcceptConnection", from_connection_id, connection_id);
  }

  void driver::connection_closed(natural_t connection_id) {
    on_connection_closed(connection_id);
    interfaces.invoke("Other.Server", "CloseConnection", connection_id);
  }

  natural_t driver::add_interface(const std::string_view interface_name, sol::table inteface_table) {
    return interfaces.register_interface_binding(interface_name, std::move(inteface_table));
  }

  void driver::http_request_received(natural_t id, const http::request& req) {
    // lua_host& lua = core_system<scripting_system>().get_lua_host();
    // sol::table req_table = lua.get_lua_state().create_table();
    // req_table["method"] = req.method.name;
    // req_table["path"] = req.path;
    // req_table["headers"] = lua.get_lua_state().create_table();
    // for (const auto& [header_name, header_value] : req.headers) {
    //   req_table["headers"][header_name] = header_value;
    // }
    // req_table["body"] = std::vector<uint8_t>(req.body);

    on_http_request_received(id, req);
    // interfaces.invoke("Other.HttpServer", "HandleHttpRequest", id, req_table);
  }

  driver::metadata driver::build_metadata() {
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

      if (std::string envrc_path = get_config_value<std::string>("scripting.envrc-path");
          !envrc_path.empty() && std::filesystem::exists(envrc_path)) {
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

    {
      PROFILE_SECTION("driver::initialize--client-on_initialize");
      on_initialize(cmd_line);
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
    PROFILE_SECTION("driver::update");
    double dt = frame_delta_time;

    driver_kernel_ptr->tick(dt);

    on_update();
    switch (current_driver_state()) {
      case driver_state::DRIVER_STATE_INITIALIZING: update_initializing(); break;

      case driver_state::DRIVER_STATE_RUNNING: {
        /// this feels gross but we if a project file was queued we want to load it before the next frame ticks
        const bool should_lock = runtime_state.queued_project_file.has_value();
        if (should_lock) {
          std::lock_guard lock(runtime_state.mutex);
          driver_kernel_ptr->get_core_system<project_system>().load_project(driver_kernel_ptr.get(), runtime_state.queued_project_file.value());
          runtime_state.queued_project_file = std::nullopt;
        }

        update_running();
      } break;

      case driver_state::DRIVER_STATE_SHUTTING_DOWN: {
        update_shutting_down();

        if (shutdown_state.ready_to_shutdown(this)) {
          on_shutdown_confirm();
          process_driver_event(driver_event::DRIVER_EVENT_READY);
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
    if (!rendering_enabled()) {
      return;
    }
    PROFILE_SECTION("driver::render");
    on_render();

    if (driver_kernel_ptr->has_core_system<rendering_system>()) {
      driver_kernel_ptr->get_core_system<rendering_system>().render(driver_kernel_ptr.get());
    }
  }

  void driver::on_project_loaded() {
    OTHER_ASSERT(driver_kernel_ptr != nullptr, "Driver kernel is not initialized.");

    auto& p = driver_kernel_ptr->get_core_system<project_system>().get_project();
    if (p.is_empty()) {
      CORE_LOG_WARN("Project loaded event triggered but project is empty. This may indicate a problem with the project loading process.");
      return;
    }

    // opt<natural_t> starting_scene_id;
    /// do this before running rc file in case rc file loads a scene
    if (auto* curr_scene = get_active_scene(); curr_scene != nullptr) {
      /// add scene to project if not in scene list
      // starting_scene_id = curr_scene->id;
      driver_kernel_ptr->get_core_system<scene_system>().unload_active_scene();
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

    /// load scenes from project
    driver_kernel_ptr->get_core_system<scene_system>().load_project_scene_graph(p);

    /// restore scene?
    // if (starting_scene_id.has_value()) {
    //   driver_kernel_ptr->get_core_system<scene_system>().set_active_scene(starting_scene_id.value());
    // }
  }

  void driver::launch_detached_process(const filepath& working_dir, const filepath& exe_name, const std::vector<std::string>& args) {
    launch_process(working_dir, exe_name, args);
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
    scene_table.set_function("attach_directional_light_to_object", &scene_interface::attach_directional_light_to_object);

    driver_table["__native_pointer"] = reinterpret_cast<std::uintptr_t>(host_driver);
    driver_table.set_function("trigger_driver_event", [host_driver](const std::string& event, sol::object data) {
      if (data.get_type() == sol::type::table) {
        CORE_LOG_ERROR("Driver event '{}' triggered with unsupported data type: {}.", event, data.get_type());
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
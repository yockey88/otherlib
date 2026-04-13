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
#include "driver/systems/asset_system.hpp"
#include "scripting/dotnet_bindings.hpp"
#include "scripting/lua_bindings.hpp"
#include "scripting/scene_interface.hpp"
#include "vm/other_device.hpp"

namespace other {

  void driver::initialize(const command_line& cmd, const subsystem_registry& registry) {
    PROFILE_SECTION("driver::initialize");
    cmd_line = cmd;
    state_machine.handle_event(driver_event::DRIVER_EVENT_START, this);
    driver_metadata = build_metadata();

    driver_kernel_ptr = make_scope<driver_kernel>(this);
    driver_kernel_ptr->load_profile(registry.get_current_profile());
    CORE_LOG_INFO("Registered Subsystems:\n{}", driver_kernel_ptr->list_systems());

    driver_kernel_ptr->initialize();

    load_client();
  }

  void driver::run() {
    PROFILE_SECTION("driver::main_loop");

    CORE_LOG_DEBUG("Entering main driver loop");
    do {
      MARK_NAMED_FRAME("driver_main_loop");
      update();

      switch (current_driver_state()) {
        case driver_state::DRIVER_STATE_INITIALIZING: update_initializing(); break;
        case driver_state::DRIVER_STATE_RUNNING: update_running(); break;
        case driver_state::DRIVER_STATE_SHUTTING_DOWN: update_shutting_down(); break;
        case driver_state::DRIVER_STATE_STOPPED: break;
        default:
          OTHER_ASSERT(false, "Driver in unknown state {}", current_driver_state());
          break;
      }

      subsystem<input_system>::get()->finalize_frame();
      if (driver_kernel_ptr->has_core_system<rendering_system>()) {
        driver_kernel_ptr->get_core_system<rendering_system>().render(driver_kernel_ptr.get());
      }

      /// \todo want to wait on asset unload for shutdown
      // if (shutdown_state.asset_manager_shutdown && shutdown_state.network_thread_shutdown) {
      //   process_driver_event(driver_event::DRIVER_EVENT_READY);
      // }
    } while (current_driver_state() != driver_state::DRIVER_STATE_STOPPED);
  }

  void driver::shutdown() {
    PROFILE_SECTION("driver::shutdown");
    {
      PROFILE_SECTION("driver::shutdown--client-on_shutdown");
      on_shutdown();
    }

    live_coroutines.clear();
    driver_kernel_ptr->shutdown();
  }

  std::pair<driver*, std::string> driver::create(const config_table& config) {
    driver* driver_instance = nullptr;

    std::string driver_path = config.dynamic_driver_rel_path.value_or("");
    std::string driver_name = "";

    /// if no path then run built-in driver/event loop with environment terminal
    if (driver_path.empty()) {
      CORE_LOG_DEBUG("Creating static driver instance");
      driver_name = config.get_value<std::string>("application.name", "static-driver");
      return { create_driver(&config), driver_name };
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

  void driver::process_driver_event(driver_event event) {
    state_machine.handle_event(event, this);
  }

  void driver::request_shutdown() {
    if (current_driver_state() == driver_state::DRIVER_STATE_SHUTTING_DOWN ||
        current_driver_state() == driver_state::DRIVER_STATE_STOPPED) {
      return;
    }

    driver_kernel_ptr->get_core_system<network_system>().begin_shutdown_sequence(driver_kernel_ptr.get());
    driver_kernel_ptr->get_core_system<asset_system>().begin_full_unload();
    live_coroutines.clear();

    on_shutdown_request();
    process_driver_event(driver_event::DRIVER_EVENT_STOP);

    if (driver_kernel_ptr->get_core_system<network_system>().get_role() == network_system::NONE) {
      on_shutdown_confirm();
      process_driver_event(driver_event::DRIVER_EVENT_READY);
    }
  }

  void driver::send_load_command(const std::string_view scene_name, natural_t scene_id, bool is_empty, bool requires_udp_binding) {
    // message cmd_msg;
    // cmd_msg.header = {
    //   .category = COMMAND,
    //   .id = ENVIRONMENT_LOAD_SCENE,
    // };

    // command_load_scene scene_cmd;
    // scene_cmd.session_id_flag = client_session_id.has_value() ? 0x01 : 0x00;
    // if (client_session_id.has_value()) {
    //   scene_cmd.session_id = client_session_id.value();
    // }

    // scene_cmd.empty_scene_flag = is_empty ? 0x01 : 0x00;
    // scene_cmd.requires_udp_binding = requires_udp_binding ? 0x01 : 0x00;
    // if (requires_udp_binding) {
    //   scene_cmd.udp_address = { network_context::kLocalhostAddress, net_context->next_available_server_port++ };
    //   scene_cmd.server_udp_address = { network_context::kLocalhostAddress, net_context->next_available_server_port++ };
    // }

    // scene_cmd.scene_name = active_scene->name;
    // cmd_msg.data.append_range(scene_cmd.as_buffer());

    // CORE_LOG_DEBUG("Sending command to network thread to load empty scene '{}'", scene_name);
    // if (scene_cmd.requires_udp_binding == 0x01) {
    //   CORE_LOG_DEBUG("Scene '{}' requires UDP binding @ [LOCAL = {}, REMOTE = {}]", scene_name, binding_point::write_string(scene_cmd.udp_address), binding_point::write_string(scene_cmd.server_udp_address));
    // }

    // /// \todo check if server is even open
    // CORE_LOG_INFO("sending ENVIRONMENT_LOAD_SCENE command for remote....");
    // send_message_and_wait_acknowledgment(std::move(cmd_msg), seconds(10), message_handler{ this, &driver::on_acknowledge_command_environment_load_scene, &driver::on_timeout_environment_load_scene });
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

  void driver::confirm_shutdown() {
    CORE_LOG_DEBUG("Confirming shutdown...");
    shutdown_state.network_thread_shutdown = true;
    on_shutdown_confirm();
    process_driver_event(driver_event::DRIVER_EVENT_READY);
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
    auto* env = subsystem<scripting_environment>::get();
    OTHER_ASSERT(env != nullptr, "scripting_environment null in load_client!");

    {
      PROFILE_SECTION("driver::initialize--client-on_initialize");
      on_initialize(cmd_line);
    }

    {
      PROFILE_SECTION("driver::initialize--client-run-envrc");
      /// run driver envrc file if it exists

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
    poll_coroutines();

    on_update();
  }

  void driver::launch_detached_process(const filepath& working_dir, const filepath& exe_name, const std::vector<std::string>& args) {
    launch_process(working_dir, exe_name, args);
  }

  void driver::post_coroutine(task coro) {
    add_live_coroutine(std::move(coro));
  }

  void driver::add_live_coroutine(task handle) {
    live_coroutines.push_back({ handle });
  }

  void driver::poll_coroutines() {
    for (auto it = live_coroutines.begin(); it != live_coroutines.end();) {
      it->handle();
      if (it->handle.coro_handle.done()) {
        it->handle.coro_handle.destroy();
        it = live_coroutines.erase(it);
      } else {
        ++it;
      }
    }
  }

}  // namespace other
/**
 * \file driver/driver.cpp
 **/
#include "driver/driver.hpp"

#include <SDL3/SDL.h>
#include <SDL3/SDL_events.h>

#include "core/defines.hpp"
#include "core/logger.hpp"
#include "thread/message.hpp"
#include "thread/messages.hpp"

#include "network/message.hpp"
#include "network/network_thread.hpp"
#include "renderer/renderer_backend.hpp"
#include "script/scripting_environment.hpp"

#include "object/camera_component.hpp"

#include "driver/driver_tasks.hpp"
#include "rendering-pipelines/default_instancing_pipeline.hpp"
#include "scripting/lua_bindings.hpp"
#include "tools/environment_console.hpp"
#include "vm/control_table.hpp"
#include "vm/other_device.hpp"
#include "vm/vm.hpp"

#include "driver_tasks.hpp"

namespace other {

  void driver::initialize(const command_line& cmd) {
    PROFILE_SECTION("driver::initialize");

    /// core setup, set state, initialize context and register core events
    cmd_line = cmd;
    state_machine.handle_event(driver_event::DRIVER_EVENT_START, this);

    initialize_network_context();
    get_event_system()->register_event("shutdown-requested");
    get_event_system()->add_listener("shutdown-requested", [this](const value& data) { request_shutdown(); });
    get_event_system()->register_event("force-load-empty-scene");
    get_event_system()->add_listener("force-load-empty-scene", std::bind_front(&driver::handle_load_empty_scene_event, this));
    get_event_system()->register_event("force-load-scene");
    get_event_system()->add_listener("force-load-scene", std::bind_front(&driver::handle_load_scene_event, this));
    get_event_system()->register_event("open-driver-ui-window");
    get_event_system()->add_listener("open-driver-ui-window", std::bind_front(&driver::handle_open_ui_window_event, this));
    get_event_system()->register_event("close-driver-ui-window");
    get_event_system()->add_listener("close-driver-ui-window", std::bind_front(&driver::handle_close_ui_window_event, this));
    get_event_system()->register_event("clear-console-output");
    get_event_system()->add_listener("clear-console-output", [this](const value& data) {
      environment_console::clear_console_output();
    });

    /// load client specific .NET
    /// \note this has to happen here because .NET can override native subsystem implementations meaning we need to load these before initializing rendering or other subsystems
    std::vector<std::string> dotnet_modules = get_config_value<std::vector<std::string>>("scripting", "dotnet-modules");
    for (const auto& module : dotnet_modules) {
      CORE_LOG_DEBUG(" - .NET module to load: {}", module);
      auto assembly = load_dotnet_module(module);
      if (assembly == nullptr) {
        CORE_LOG_ERROR("Failed to load .NET module: {}", module);
      }

      loaded_dotnet_modules.push_back(assembly);
    }

    auto* env = subsystem<scripting_environment>::get();
    OTHER_ASSERT(env != nullptr, "scripting_environment null in initialize!");
    bind_otherlib_driver_lua_functions(env->get_lua_host(), this);
    // bind_scene_object_interface_lua_functions(env->get_lua_host(), this);

    if (rendering_enabled()) {
      initialize_rendering();
    }

    vm::initialize_device(&core_device);
    vm::activate_builtin_control_table(&core_device, OTHER_CONTROL_TABLE_V000);
    core_device.stopped = false;
    core_device.host_driver = this;

    project_scene_graph = make_scope<scene_graph>();
    asset_mgr = make_scope<asset_handler>(net_context->io_context);

    load_client();
    start_network();
  }

  void driver::run() {
    PROFILE_SECTION("driver::main_loop");
    while (current_driver_state() != driver_state::DRIVER_STATE_STOPPED) {
      update();
      if (current_driver_state() == driver_state::DRIVER_STATE_STOPPED) {
        break;
      }

      switch (current_driver_state()) {
        case driver_state::DRIVER_STATE_INITIALIZING: update_initializing(); break;
        case driver_state::DRIVER_STATE_RUNNING: update_running(); break;
        case driver_state::DRIVER_STATE_SHUTTING_DOWN: update_shutting_down(); break;
        default:
          OTHER_ASSERT(false, "Driver in unknown state {}", current_driver_state());
          break;
      }

      render();
    }
  }

  void driver::shutdown() {
    PROFILE_SECTION("driver::shutdown");
    {
      PROFILE_SECTION("driver::shutdown--client-on_shutdown");
      on_shutdown();
    }

    if (rendering_enabled()) {
      shutdown_rendering();
    }

    core_device.stopped = true;
    vm::shutdown_device(&core_device);

    for (auto& module : loaded_dotnet_modules) {
      unload_dotnet_module(module);
    }
    loaded_dotnet_modules.clear();

    live_coroutines.clear();

    asset_mgr = nullptr;
    project_scene_graph = nullptr;

    net_context->net_thread = nullptr;
    net_context->events = nullptr;
    net_context = nullptr;
  }

  std::pair<driver*, std::string> driver::create(const config_table& config) {
    driver* driver_instance = nullptr;

    std::string driver_path = config.dynamic_driver_rel_path.value_or("");
    std::string driver_name = "";

    /// if no path then run built-in driver/event loop with environment terminal
    if (driver_path.empty()) {
      CORE_LOG_ERROR("No driver path specified, using default driver.");
      return { nullptr, "" };
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
    }

    return { driver_instance, driver_name };
  }

  void driver::destroy(const std::string& name, driver* instance) {
    OTHER_ASSERT(instance != nullptr, "Cannot destroy a null driver instance.");

    library_handle* lib_handle = plugin::get_plugin_library(name);
    if (lib_handle == nullptr) {
      CORE_LOG_ERROR("Failed to get plugin library: {}", name);
      return;
    }

    auto sym_res = lib_handle->get_symbol("destroy_driver");
    if (!sym_res.has_value()) {
      CORE_LOG_ERROR("Failed to get symbol 'destroy_driver' from plugin '{}'", name);
      return;
    }
    symbol& sym = sym_res.value();
    if (sym.address == nullptr) {
      CORE_LOG_ERROR("Failed to load symbol 'destroy_driver' from plugin '{}'", name);
      return;
    }

    CORE_LOG_DEBUG("calling 'destroy_driver' from plugin [{}]", name);
    sym.get_function<void (*)(driver*)>()(instance);
    plugin::unload_plugin_library(name);
  }

  void driver::on_initialize_rendering(scope<renderer>& renderer_ptr) {
    renderer_ptr->add_pipeline<default_instancing_pipeline>("Rendering Pipeline");
  }

  void driver::on_shutdown_rendering() {
    get_renderer_instance().remove_pipeline("Rendering Pipeline");
  }

  void driver::catch_signal(int signum) {
    if (signum == SIGINT || signum == SIGTERM) {
      CORE_LOG_INFO("Received signal {}, shutting down driver...", signum);
      request_shutdown();
    } else {
      CORE_LOG_WARN("Received unhandled signal {}", signum);
    }
  }

  void driver::request_shutdown() {
    if (current_driver_state() == driver_state::DRIVER_STATE_SHUTTING_DOWN ||
        current_driver_state() == driver_state::DRIVER_STATE_STOPPED) {
      return;
    }

    net_context->signals.cancel();
    if (!net_context->io_context.stopped()) {
      net_context->io_context.stop();
    }

    asset_mgr->purge_stores();

    for (auto& ack : ack_list.pending_acks) {
      ack.timer.cancel();
    }
    ack_list.pending_acks.clear();

    for (auto& response : resp_list.pending_responses) {
      response.timer.cancel();
    }
    resp_list.pending_responses.clear();

    live_coroutines.clear();

    CORE_LOG_DEBUG("Sending shutdown request to network thread...");
    message msg;
    msg.header = {
      .category = COMMAND,
      .id = SHUTDOWN_REQUEST,
    };

    send_message_and_wait_acknowledgment(std::move(msg), std::chrono::seconds(3), message_handler{ this, &driver::on_ack_shutdown_request_network_thread, &driver::on_timeout_shutdown_request_network_thread });

    on_shutdown_request();
    process_driver_event(driver_event::DRIVER_EVENT_STOP);
  }

  natural_t driver::create_new_scene(const std::string_view name) {
    OTHER_ASSERT(project_scene_graph != nullptr, "Project scene graph is not initialized.");
    auto [scene_id, ptr] = project_scene_graph->create_new_scene(name);
    OTHER_ASSERT(ptr != nullptr, "Failed to create new scene: {}", name);
    CORE_LOG_INFO("Created new scene [{}:{}]", scene_id, name);
    return scene_id;
  }

  scene* driver::get_scene(natural_t id) {
    OTHER_ASSERT(project_scene_graph != nullptr, "Project scene graph is not initialized.");
    return project_scene_graph->get_scene(id);
  }

  renderer& driver::get_renderer_instance() {
    OTHER_ASSERT(renderer_ptr != nullptr, "Renderer is not initialized in driver.");
    return *renderer_ptr;
  }

  void driver::trigger_event(const std::string_view name, const value& data) {
    OTHER_ASSERT(net_context != nullptr, "Network context is not initialized in driver.");
    OTHER_ASSERT(net_context->events != nullptr, "Event system is not initialized in driver.");
    net_context->events->set_user_data(name, data);
    net_context->events->trigger_event(name);
  }

  void driver::write_id_at_address(uint16_t address, natural_t id) {
    OTHER_ASSERT(address + sizeof(natural_t) <= other_command_device::kMemorySize, "Address out of bounds: {}", address);
    core_device.write_u64_at(address, id);
  }

  void driver::emit_instruction(const instruction& op) {
    auto data = std::span(reinterpret_cast<const uint8_t*>(&op.opcode), sizeof(op.opcode));
    vm::load_bytes_to_address(&core_device, core_device.program_load_cursor, data.data(), data.size());
    core_device.program_load_cursor += data.size();
  }

  void driver::driver_step_device() {
    PROFILE_SECTION("driver::driver_step_device");
    core_device.current_instruction = *(uint32_t*)&core_device.memory->at(core_device.pc);
    if (core_device.current_instruction.opcode == 0x00000000) {
    } else {
      core_device.pc += other_command_device::kOpCodeSize;

      uint8_t instr_nib = core_device.current_instruction.category_nibble();
      core_device.control_table[instr_nib](&core_device);
      vm::update_device_timers(&core_device);
    }
  }

  natural_t driver::begin_asset_load(const filepath& asset_path, std::function<void(natural_t asset_id)> on_loaded) {
    OTHER_ASSERT(asset_mgr != nullptr, "Asset manager is not initialized in driver.");
    natural_t asset_id = asset_mgr->load_asset(asset_path);
    loading_asset_ids.push_back({
      .asset_id = asset_id,
      .on_loaded = on_loaded,
    });
    return asset_id;
  }

  scene* driver::get_active_scene() {
    return active_scene;
  }

  void driver::set_scene_to_active(natural_t scene_id) {
    CORE_LOG_DEBUG("Setting scene [{}] as active scene in driver.", scene_id);
    if (active_scene != nullptr && active_scene->id == scene_id) {
      CORE_LOG_WARN("Scene [{}] is already the active scene.", scene_id);
      return;
    }

    if (active_scene != nullptr) {
      get_event_system()->cancel_event("scene-update");
    }

    active_scene = project_scene_graph->get_scene(scene_id);
    auto& storage = active_scene->get_storage();
    if (storage.sandbox["OnSceneActivate"].valid()) {
      CORE_LOG_DEBUG("Calling 'OnSceneActivate' for scene [{}:{}]", active_scene->id, active_scene->name);
      sol::protected_function on_scene_activate_fn = storage.sandbox["OnSceneActivate"];
      sol::protected_function_result result = on_scene_activate_fn();
      if (!result.valid()) {
        CORE_LOG_ERROR("Failed to execute 'OnSceneActivate' for scene [{}:{}]", active_scene->id, active_scene->name);
        sol::error err = result;
        CORE_LOG_ERROR("Lua Error: {}", err.what());
      }
    }

    /// about 0.02 seconds per update
    get_event_system()->register_timed_event("scene-update", milliseconds(20), true);
    get_event_system()->add_listener("scene-update", [this](const value& data) {
      if (active_scene != nullptr) {
        active_scene->fixed_update(frame_delta_time.get_no_update());
      }
    });
  }

  void driver::unload_active_scene() {
    if (active_scene == nullptr) {
      CORE_LOG_WARN("No active scene to unload in driver.");
      return;
    }
    CORE_LOG_DEBUG("Unloading active scene [{}:{}] from driver.", active_scene->id, active_scene->name);
    /// \todo serialize or something before removing?
    project_scene_graph->remove_scene(active_scene->id);
    active_scene = nullptr;
  }

  void driver::initialize_network_context() {
    net_context = std::make_unique<network_context>();

    struct signal_catcher {
      signal_catcher(driver* driver_ptr)
          : driver_ptr(driver_ptr) {}
      void catch_signal(std::error_code ec, int signum) {
        if ((ec && ec == asio::error::operation_aborted) ||
            driver_ptr == nullptr || driver_ptr->net_context == nullptr) {
          return;
        }

        if (!ec) {
          driver_ptr->catch_signal(signum);
        } else {
          CORE_LOG_ERROR("Error while waiting for signal: {}", ec.message());

          if (driver_ptr->current_driver_state() == driver_state::DRIVER_STATE_RUNNING) {
            driver_ptr->net_context->signals.async_wait(std::bind_front(&signal_catcher::catch_signal, this));
          }
        }
      }

      driver* driver_ptr = nullptr;
    };

    static signal_catcher catcher{ this };
    net_context->signals.async_wait(std::bind_front(&signal_catcher::catch_signal, &catcher));

    net_context->events = make_scope<event_system>(net_context->io_context);
    net_context->net_thread = make_scope<network_thread>(net_context->net_thread_message_bus);
    net_context->net_thread->launch();
    net_context->net_thread_message_bus.register_thread();
  }

  void driver::load_client() {
    auto* env = subsystem<scripting_environment>::get();
    OTHER_ASSERT(env != nullptr, "scripting_environment null in load_client!");

    /// run driver envrc file if it exists
    std::string envrc_path = get_config_value<std::string>("scripting", "envrc-path");
    if (!envrc_path.empty() && std::filesystem::exists(envrc_path)) {
      /// this one has to be loaded into the host without the sandboxing of the environment
      ///  as this is supposed to be the user's customization of the environment
      auto& lua_host = env->get_lua_host();
      sol::state& lua_state = lua_host.get_lua_state();
      try {
        auto script = lua_state.script_file(envrc_path);
        if (!script.valid()) {
          sol::error err = script;
          CORE_LOG_ERROR("Failed to run driver environment runtime script: {}\n{}", envrc_path, err.what());
        } else {
          envrc = lua_host.load_file(envrc_path);
          CORE_LOG_INFO("Successfully ran driver environment runtime script: {}", envrc_path);
        }
      } catch (...) {
        CORE_LOG_ERROR("Failed to run driver environment runtime script: {}", envrc_path);
      }
    }

    {
      PROFILE_SECTION("driver::initialize--client-on_initialize");
      on_initialize(cmd_line);
    }
  }

  void driver::start_network() {
    /// now we kick off main networking session
    bool force_disable_network = get_config_value<bool>("network-thread", "force-disable", false);
    bool network_thread_active = !force_disable_network && net_context->net_thread != nullptr;

    std::string role = get_config_value<std::string>("application", "role", "client");
    if (role != "client" && role != "server") {
      CORE_LOG_WARN("Unknown application role '{}', defaulting to 'client'", role);
      role = "client";
    }

    if (role == "client") {
      primary_role = driver_role::CLIENT;
    } else if (role == "server") {
      primary_role = driver_role::SERVER;
    }

    if (network_thread_active && primary_role == driver_role::CLIENT) {
      message connect_msg;
      connect_msg.header = {
        .category = COMMAND,
        .id = SESSION_CONNECT_TO,
      };

      command_session_connect_to conn_cmd;
      conn_cmd.address = net_context->main_binding_point;
      connect_msg.data.append_range(conn_cmd.as_buffer());
      send_to_network_thread(std::move(connect_msg));
    } else if (network_thread_active && primary_role == driver_role::SERVER) {
      message msg;
      msg.header = {
        .category = COMMAND,
        .id = SESSION_LISTEN_FOR,
      };

      command_session_listen_at listen_cmd;
      listen_cmd.address = net_context->main_binding_point;
      msg.data.append_range(listen_cmd.as_buffer());

      send_message_and_wait_acknowledgment(std::move(msg), std::chrono::seconds(10), message_handler{ this, &driver::on_ack_session_listen_for_network_thread, &driver::on_timeout_session_listen_for_network_thread });
    }
  }

  void driver::initialize_rendering() {
    renderer_ptr = get_renderer();
    on_initialize_rendering(renderer_ptr);

    initialize_ui();
  }

  void driver::initialize_ui() {
    driver_ui_ptr = make_scope<driver_ui>(this);
    driver_ui_ptr->initialize();
    on_initialize_ui(driver_ui_ptr);

    auto open_windows = configuration().get_value<std::vector<std::string>>("ui.open-windows", std::vector<std::string>{});
    for (const auto& window_name : open_windows) {
      value val = window_name;
      handle_open_ui_window_event(val);
    }
  }

  void driver::shutdown_rendering() {
    shutdown_ui();
    on_shutdown_rendering();
  }

  void driver::shutdown_ui() {
    on_shutdown_ui();
    driver_ui_ptr->shutdown();
    driver_ui_ptr = nullptr;
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

  void driver::open_ui_window(driver_ui::builtin_window_type type) {
    if (!rendering_enabled()) {
      return;
    }

    OTHER_ASSERT(driver_ui_ptr != nullptr, "Driver UI is not initialized.");
    driver_ui_ptr->open_builtin_window(type);
  }

  void driver::close_ui_window(driver_ui::builtin_window_type type) {
    if (!rendering_enabled()) {
      return;
    }

    OTHER_ASSERT(driver_ui_ptr != nullptr, "Driver UI is not initialized.");
    driver_ui_ptr->close_builtin_window(type);
  }

  void driver::process_driver_event(driver_event event) {
    state_machine.handle_event(event, this);
  }

  void driver::update() {
    PROFILE_SECTION("driver::update");
    double dt = frame_delta_time;

    pump_events();
    poll_coroutines();

    /// check assets being loaded
    for (auto it = loading_asset_ids.begin(); it != loading_asset_ids.end();) {
      natural_t asset_id = it->asset_id;
      if (asset_mgr->get_asset_state(asset_id) == asset_state::LOADED) {
        if (it->on_loaded) {
          it->on_loaded(asset_id);
        }

        it = loading_asset_ids.erase(it);
      } else {
        ++it;
      }
    }

    if (environment_console::is_initialized()) {
      environment_console::poll();
    }

    /// we could propbably assert here if this is null
    if (net_context != nullptr) {
      net_context->io_context.poll();
      if (net_context->io_context.stopped()) {
        net_context->io_context.restart();
      }
    }

    auto msg_opt = net_context->net_thread_message_bus.receive_message();
    if (msg_opt.has_value()) {
      process_network_thread_messages(std::move(*msg_opt));
    }

    if (active_scene != nullptr) {
      active_scene->update(dt);
      active_scene->late_update(dt);
    }

    on_update();
  }

  void driver::render() {
    if (!rendering_enabled()) {
      return;
    }

    render_data data = {};
    if (active_scene != nullptr) {
      data = active_scene->prepare_render_data(asset_mgr);
      get_renderer_instance().begin_frame(&data);
    } else {
      get_renderer_instance().begin_frame(nullptr);
    }
    get_renderer_instance().render();

    on_render();
    render_ui();

    get_renderer_instance().end_frame();
  }

  void driver::render_ui() {
    if (!rendering_enabled()) {
      return;
    }

    get_renderer_instance().begin_ui_frame();

    driver_ui_ptr->render();

    on_ui_render();
    get_renderer_instance().end_ui_frame();
  }

  void driver::pump_events() {
    PROFILE_SECTION("driver::pump_events");

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      switch (event.type) {
        case SDL_EVENT_QUIT:
        case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
          if (!(current_driver_state() == driver_state::DRIVER_STATE_SHUTTING_DOWN ||
                current_driver_state() == driver_state::DRIVER_STATE_STOPPED)) {
            get_event_system()->trigger_event("shutdown-requested");
          }
          break;

        default: {
        } break;
      }

      subsystem<renderer_backend>::get()->handle_event(&event);
      on_event(&event);
    }
  }

  void driver::handle_request_session_information(integer_t session_id, message&& msg) {
    session_information_response response;
    response.name = get_project_name();
    response.executable = get_current_exe_full_path();
    response.working_directory = std::filesystem::current_path().string();

    CORE_LOG_TRACE("Session Information Response for session [{}]:", session_id);
    CORE_LOG_TRACE("   - Name: {}", response.name);
    CORE_LOG_TRACE("   - Executable: {}", response.executable);
    CORE_LOG_TRACE("   - Working Directory: {}", response.working_directory);

    response.name_flag = response.name != "";
    response.executable_flag = response.executable != "";
    response.working_directory_flag = response.working_directory != "";

    message resp_msg;
    resp_msg.header = {
      .category = RESPONSE,
      .id = SESSION_INFORMATION,
    };

    resp_msg.data.append_range(response.as_buffer());

    message tx_msg;
    tx_msg.header = {
      .category = COMMAND,
      .id = SESSION_TX_MESSAGE,
    };

    command_session_tx_message tx_session_msg;
    tx_session_msg.session_id = session_id;
    tx_session_msg.msg = std::move(resp_msg);
    tx_msg.data.append_range(tx_session_msg.as_buffer());

    send_message_and_detach_response(std::move(tx_msg), nullptr);
  }

  void driver::handle_response_session_information(integer_t session_id, message&& msg) {
    session_information_response response = other_message_spec::parse<session_information_response>(std::span(msg.data));
    handle_session_information_response(session_id, std::move(response));
  }

  void driver::on_acknowledge_command_environment_load_scene(message_header header, std::span<const uint8_t> data) {
    CORE_LOG_DEBUG("Received acknowledgment for ENVIRONMENT_LOAD_SCENE command (header: {})", header);

    /// expect a udp_binding_information structure in data
    acknowledgement ackmsg = other_message_spec::parse<acknowledgement>(data);
    if (ackmsg.ack_nack != 0x01) {
      CORE_LOG_ERROR("ENVIRONMENT_LOAD_SCENE command was NACKed by server (header: {})", header);
      return;
    }

    if (ackmsg.extra_data_length == 0) {
      CORE_LOG_ERROR("ENVIRONMENT_LOAD_SCENE acknowledgment from server missing UDP binding information (header: {})", header);
      return;
    }
    if (ackmsg.extra_data.size() < ackmsg.extra_data_length) {
      CORE_LOG_ERROR("ENVIRONMENT_LOAD_SCENE acknowledgment from server has invalid UDP binding information length (header: {})", header);
      return;
    }

    udp_binding_information binding_info = other_message_spec::parse<udp_binding_information>(std::span<const uint8_t>(ackmsg.extra_data));

    CORE_LOG_INFO("ENVIRONMENT_LOAD_SCENE command acknowledged by [session {}]. Requesting UDP stream", ackmsg.session_id <= 0 ? "<self>" : std::to_string(ackmsg.session_id));
    CORE_LOG_INFO("  - Requires UDP Binding: {}", binding_info.endpoint.port != 0 && binding_info.remote_endpoint.port != 0 ? "Yes" : "No");
    if (binding_info.endpoint.port != 0 || binding_info.remote_endpoint.port != 0) {
      CORE_LOG_INFO("  - Check-in Hash: {}", binding_info.check_in_hash);
      CORE_LOG_INFO("  - Local Endpoint: [{}]", binding_point::write_string(binding_info.endpoint));
      CORE_LOG_INFO("  - Remote Endpoint: [{}]", binding_point::write_string(binding_info.remote_endpoint));
      request_scene_udp_binding(binding_info);
    }
  }

  void driver::on_timeout_environment_load_scene(message_header header) {
    CORE_LOG_ERROR("Timeout while waiting for acknowledgment of ENVIRONMENT_LOAD_SCENE command (header: {})", header);
    CORE_LOG_WARN("Does the active project have a scene using that name already?");

    // unload_active_scene();
  }

  void driver::request_scene_udp_binding(udp_binding_information address) {
    message udp_request;
    udp_request.header = {
      .category = REQUEST,
      .id = NEW_UDP_STREAM_BINDING,
    };

    CORE_LOG_INFO("Requesting new UDP stream binding @ address [{}]", binding_point::write_string(address.endpoint));
    CORE_LOG_INFO("  - Remote endpoint: [{}]", binding_point::write_string(address.remote_endpoint));

    new_udp_stream_binding_request req;
    req.address = address.endpoint;
    req.remote_address = address.remote_endpoint;
    udp_request.data.append_range(req.as_buffer());

    send_message_and_wait_response(std::move(udp_request), seconds(5), message_handler{ this, &driver::on_respond_new_udp_stream_binding, &driver::on_timeout_new_udp_stream_binding });
  }

  void driver::on_respond_new_udp_stream_binding(message_header header, std::span<const uint8_t> data) {
    OTHER_ASSERT(active_scene != nullptr, "No active scene to set UDP handle on.");
    CORE_LOG_DEBUG("Received response for NEW_UDP_STREAM_BINDING (header: {})", header);

    new_udp_stream_binding_response resp = other_message_spec::parse<new_udp_stream_binding_response>(data);
    integer_t binding_id = resp.binding_id;

    if (resp.binding_id < 0) {
      CORE_LOG_ERROR("Received invalid UDP stream binding ID from server: {}", resp.binding_id);
      return;
    }

    active_streams.push_back({ .stream_id = binding_id });

    CORE_LOG_INFO("Received UDP stream binding from server: {}", resp.binding_id);
    active_scene->update_stream_id = binding_id;

    if (primary_role == driver_role::CLIENT) {
      CORE_LOG_DEBUG("No check-in required for UDP stream binding ID: {}", binding_id);

      message msg;
      msg.header = {
        .category = COMMAND,
        .id = STREAM_SEND_UDP_DATAGRAM,
      };

      command_stream_send_udp_datagram udp_msg;
      udp_msg.stream_id = binding_id;
      udp_msg.datagram.type = udp_packet_type::UDP_CHECK_IN;
      udp_msg.datagram.packet.check_in = {
        .hash = active_scene->id,
      };

      msg.data.append_range(udp_msg.as_buffer());
      send_message_and_detach_response(std::move(msg), { this });

    } else {
      CORE_LOG_DEBUG("UDP binding ID: {} waiting for check-in from server.", binding_id);
    }
  }

  void driver::on_timeout_new_udp_stream_binding(message_header header) {
    CORE_LOG_ERROR("Timeout while waiting for NEW_UDP_STREAM_BINDING response (header: {})", header);
  }

  void driver::handle_session_event_rx_message(message&& msg) {
    auto bytes = std::span(msg.data);

    session_event_rx_message session_msg = other_message_spec::parse<session_event_rx_message>(bytes);
    integer_t session_id = session_msg.session_id;
    message rx_msg = std::move(session_msg.msg);

    switch (rx_msg.header.category) {
      case NOTIFICATION:
        switch (rx_msg.header.id) {
          default:
            CORE_LOG_WARN("Received unknown session event notification message ID: {}", rx_msg.header.id);
            break;
        }
        break;

      case ACKNOWLEDGEMENT:
        switch (rx_msg.header.id) {
          default:
            CORE_LOG_WARN("Received unknown session event acknowledgment message ID: {}", rx_msg.header.id);
            break;
        }
        break;

      case CONTROL:
        switch (rx_msg.header.id) {
          default:
            CORE_LOG_WARN("Received unknown session event control message ID: {}", rx_msg.header.id);
            break;
        }
        break;

      case COMMAND:
        switch (rx_msg.header.id) {
          case ENVIRONMENT_LOAD_SCENE: handle_command_environment_load_scene(session_id, std::move(rx_msg)); return;
          default:
            CORE_LOG_WARN("Received unknown session event command message ID: {}", rx_msg.header.id);
            break;
        }
        break;

      case REQUEST:
        switch (rx_msg.header.id) {
          case SESSION_INFORMATION: handle_request_session_information(session_id, std::move(rx_msg)); return;
          default:
            CORE_LOG_WARN("Received unknown session event request message ID: {}", rx_msg.header.id);
            break;
        }
        break;

      case RESPONSE:
        switch (rx_msg.header.id) {
          case SESSION_INFORMATION: handle_response_session_information(session_id, std::move(rx_msg)); return;
          default:
            CORE_LOG_WARN("Received unknown session event response message ID: {}", rx_msg.header.id);
            break;
        }
        break;

      case SESSION_EVENT:
        switch (rx_msg.header.id) {
          case SESSION_RX_MESSAGE: return;
          default:
            CORE_LOG_WARN("Received unknown session event message ID: {}", rx_msg.header.id);
            break;
        }
        break;

      case ERROR_ALERT:
        switch (rx_msg.header.id) {
          default:
            CORE_LOG_WARN("Received unknown session event error alert message ID: {}", rx_msg.header.id);
            break;
        }
        break;

      default:
        CORE_LOG_WARN("Received unknown session event message category: {}", rx_msg.header.category);
        break;
    }
  }

  void driver::handle_notification_stream_receive_udp_datagram(message&& msg) {
    notification_stream_rx_datagram udp_msg = other_message_spec::parse<notification_stream_rx_datagram>(msg.data);
    integer_t stream_id = udp_msg.stream_id;
    udp_datagram datagram = udp_msg.datagram;

    CORE_LOG_DEBUG("Received UDP datagram on stream ID: {} of type: {}", stream_id, static_cast<uint8_t>(datagram.type));

    switch (datagram.type) {
      case udp_packet_type::UDP_CHECK_IN: {
        udp_check_in check_in = datagram.packet.check_in;
        if (check_in.hash != active_scene->id) {
          CORE_LOG_ERROR("Received UDP check-in with invalid hash: {} (expected: {})", check_in.hash, active_scene->id);
        } else {
          CORE_LOG_INFO("Received valid UDP check-in on stream ID: {} for scene ID: {}", stream_id, check_in.hash);
        }
      } break;

      default:
        CORE_LOG_WARN("Received unknown UDP packet type: {} on stream ID: {}", static_cast<uint8_t>(datagram.type), stream_id);
        break;
    }
  }

  void driver::handle_notification_session_check_in(message&& msg) {
    notification_session_check_in check_in = other_message_spec::parse<notification_session_check_in>(msg.data);
    integer_t session_id = check_in.session_id;
    if (session_id < 0) {
      CORE_LOG_ERROR("Invalid session ID received in session check-in notification: {}", session_id);
      return;
    }

    if (session_id == 0) {
      CORE_LOG_ERROR("Received session check-in notification with session ID 0.");
      return;
    }

    CORE_LOG_INFO("Session [{}] checked in.", session_id);
    if (primary_role == driver_role::SERVER) {
      /// new connection so it will not be in pending, but check for collision with ids
      if (auto itr = std::ranges::find_if(app_list.pending_apps, [&](const application_list::other_application& app) { return session_id == app.id; });
          itr != app_list.pending_apps.end()) {
        /// \todo handle collision, start (new-id-protocol)
        ///      for now error out
        CORE_LOG_ERROR("Server received a session check in for an already pending Other application : {}", session_id);
        return;
      }

      application_list::other_application* app = nullptr;
      auto itr = app_list.other_apps.find(session_id);
      if (itr != app_list.other_apps.end()) {
        /// \todo handle collision, start (new-id-protocol)
        ///      for now error out
        CORE_LOG_ERROR("Server received a session check in for an already connected Other application : {}", session_id);
        return;
      }

      app_list.other_apps.emplace(session_id, application_list::other_application{ .id = session_id });
      app = &app_list.other_apps[session_id];
      register_other_application(session_id, app);
    } else if (primary_role == driver_role::CLIENT) {
      client_session_id = session_id;
    }
  }

  void driver::handle_notification_session_closed(message&& msg) {
    notification_session_closed closed = other_message_spec::parse<notification_session_closed>(msg.data);
    integer_t session_id = closed.session_id;
    if (session_id < 0) {
      CORE_LOG_ERROR("Invalid session ID received in session closed notification: {}", session_id);
      return;
    }

    CORE_LOG_INFO("Session [{}] closed.", session_id);
    on_notification_session_closed(session_id);
  }

  void driver::handle_acknowledgement_ack(message&& msg) {
    CORE_LOG_DEBUG("  - ACK");
    acknowledgement ackmsg = other_message_spec::parse<acknowledgement>(msg.data);
    message_header acked_header = ackmsg.acked_header;

    auto itr = std::find_if(ack_list.pending_acks.begin(), ack_list.pending_acks.end(), [&](const acknowledgement_list::pending_ack& ack) {
      return acked_header == ack.header;
    });
    if (itr == ack_list.pending_acks.end()) {
      CORE_LOG_ERROR("Received acknowledgment for unknown message {}", acked_header);
      return;
    }

    CORE_LOG_DEBUG("Acknowledgment received for message {}", acked_header);
    itr->timer.cancel();

    if (itr->handler.handle_msg) {
      CORE_LOG_TRACE("Invoking acknowledgment callback for message {}", acked_header);
      (this->*itr->handler.handle_msg)(acked_header, msg.data);
    }
    ack_list.pending_acks.erase(itr);
  }

  void driver::on_ack_shutdown_request_network_thread(message_header header, const std::span<const uint8_t> data) {
    CORE_LOG_DEBUG("Network thread acknowledged shutdown request");
    net_context->net_thread->shutdown();
    on_shutdown_confirm();
    process_driver_event(driver_event::DRIVER_EVENT_READY);
  }

  void driver::on_timeout_shutdown_request_network_thread(message_header header) {
    /// force shutdown
    CORE_LOG_WARN("Timeout waiting for network thread to acknowledge shutdown request, forcing shutdown...");
    net_context->net_thread->force_shutdown();
    on_shutdown_confirm();
    process_driver_event(driver_event::DRIVER_EVENT_READY);
  }

  void driver::on_ack_session_listen_for_network_thread(message_header header, const std::span<const uint8_t> data) {
    CORE_LOG_INFO("Network thread acknowledged event request at session check in for session [{}]", header.id);

    /// register event for thread check in
    natural_t event_id = get_event_system()->register_timed_event("status-check:[network-thread]", seconds(1), /* recurring = */ true);
    if (event_id == 0) {
      CORE_LOG_ERROR("Failed to register event for network thread check-in");
      return;
    }
    get_event_system()->add_listener(event_id, [this](const value& ec) {
      message msg;
      msg.header = {
        .category = CONTROL,
        .id = PING,
      };

      control_ping ping_msg;

      net_context->netw_thread_heartbeat_timeout_id = set_timeout(milliseconds(250), [this](natural_t timeout_id) {
        CORE_LOG_ERROR("Network thread failed to respond to PING within timeout period");
        /// handle_network_thread_unresponsive();
      });

      send_to_network_thread(std::move(msg));
    });
    /// ready : initializing -> running
    process_driver_event(driver_event::DRIVER_EVENT_READY);
  }

  void driver::on_timeout_session_listen_for_network_thread(message_header header) {
    CORE_LOG_WARN("Network thread timed out waiting for event request at session check in for session [{}]", header.id);
  }

  void driver::handle_control_ping(message&& msg) {
    control_ping ping_msg = other_message_spec::parse<control_ping>(msg.data);
    if (ping_msg.session_id == 0) {
      // /// respond to ping
      // message pong_msg;
      // pong_msg.header = {
      //   .category = CONTROL,
      //   .id = PONG,
      // };

      // control_pong pong;
      // pong.session_id = 0;
      // pong_msg.data.append_range(pong.as_buffer());

      // send_to_network_thread(std::move(pong_msg));
    }
  }

  void driver::handle_control_pong(message&& msg) {
    control_pong pong_msg = other_message_spec::parse<control_pong>(msg.data);
    if (pong_msg.session_id == 0) {
      clear_timeout(net_context->netw_thread_heartbeat_timeout_id);
    }
    /// else handle real session
    else {
      /// \todo
    }
  }

  void driver::handle_command_environment_load_scene(integer_t session_id, message&& msg) {
    command_load_scene scene_cmd = other_message_spec::parse<command_load_scene>(msg.data);

    natural_t scene_id = create_empty_scene(scene_cmd.scene_name);
    set_scene_to_active(scene_id);
    OTHER_ASSERT(active_scene != nullptr, "Failed to set active scene after loading empty scene");

    /// if we received this them we are the 'server' part of the UDP stream (i.e. currently hosting the scene)
    /// so we name these in terms of us being the server
    /// \todo check if these are ok, and if not response with better ones
    udp_binding_information binding_info;
    binding_info.endpoint = scene_cmd.server_udp_address;
    binding_info.remote_endpoint = scene_cmd.udp_address;
    binding_info.check_in_hash = active_scene->id;

    CORE_LOG_DEBUG("Suggested Scene Endpoints local = [{}], remote = [{}]", binding_point::write_string(binding_info.endpoint), binding_point::write_string(binding_info.remote_endpoint));
    bool request_udp_binding = scene_cmd.requires_udp_binding == 0x01;
    if (request_udp_binding) {
      request_scene_udp_binding(binding_info);
    } else {
      CORE_LOG_DEBUG("Scene '{}' does not require UDP binding.", scene_cmd.scene_name);
    }

    /// first acknowledge that we loaded the scene
    {
      message ack_msg;
      ack_msg.header = {
        .category = ACKNOWLEDGEMENT,
        .id = ACK,
      };

      acknowledgement ackmsg;
      ackmsg.session_id = session_id;
      ackmsg.acked_header = msg.header;
      ackmsg.ack_nack = 0x01;

      /// \todo send back final addressess, currently just echoing what was sent
      udp_binding_information client_binding_info;
      client_binding_info.endpoint = scene_cmd.udp_address;
      client_binding_info.remote_endpoint = scene_cmd.server_udp_address;
      client_binding_info.check_in_hash = active_scene->id;
      ackmsg.extra_data.append_range(client_binding_info.as_buffer());
      ack_msg.data.append_range(ackmsg.as_buffer());

      CORE_LOG_DEBUG("acknowledging ENVIRONMENT_LOAD_SCENE command");
      message tx_msg;
      tx_msg.header = {
        .category = COMMAND,
        .id = SESSION_TX_MESSAGE,
      };

      command_session_tx_message tx_session_msg;
      tx_session_msg.session_id = session_id;
      tx_session_msg.msg = std::move(ack_msg);
      tx_msg.data.append_range(tx_session_msg.as_buffer());

      send_to_network_thread(std::move(tx_msg));
    }

    /// scene is empty, so we are already up too date
    if (scene_cmd.empty_scene_flag == 0x01) {
      CORE_LOG_DEBUG("Scene '{}' is empty, no scene data to request.", scene_cmd.scene_name);
      return;
    }
    active_scene->connect_remote_session(session_id);
  }

  void driver::session_check_in_request(integer_t session_id) {
    message msg;
    msg.header = {
      .category = REQUEST,
      .id = SESSION_CHECK_IN,
    };

    struct session_check_in_request request;
    request.session_id = session_id;
    msg.data.append_range(request.as_buffer());
    // send_message_and_detach_response(std::move(msg), std::bind_front(&server::on_respond_session_check_in_network_thread, this));
  }

  void driver::session_application_information_request(integer_t session_id, application_list::other_application* app) {
    if (app == nullptr) {
      auto itr = app_list.other_apps.find(session_id);
      if (itr == app_list.other_apps.end()) {
        CORE_LOG_ERROR("Cannot send session information request to unknown Other application session ID {}", session_id);
        return;
      }
      app = &itr->second;
    }
    if (app == nullptr) {
      CORE_LOG_ERROR("Application pointer is null for session ID {}", session_id);
      return;
    }

    session_information_request request;
    request.project_data_flag = !app->name.has_value() && !app->executable.has_value() && !app->working_directory.has_value();
    if (request.project_data_flag == 0) {
      request.name_flag = app->name.has_value() ? 1 : 0;
      request.executable_flag = app->executable.has_value() ? 1 : 0;
      request.working_directory_flag = app->working_directory.has_value() ? 1 : 0;
    }

    if (request.project_data_flag == 0 && request.name_flag == 0 && request.executable_flag == 0 && request.working_directory_flag == 0) {
      print_session_information(app);
      return;
    } else {
      CORE_LOG_INFO("Requesting session information from Other application session [{}]", session_id);
    }

    message session_msg;
    session_msg.header = {
      .category = REQUEST,
      .id = SESSION_INFORMATION,
    };
    session_msg.data.append_range(request.as_buffer());

    message msg;
    msg.header = {
      .category = COMMAND,
      .id = SESSION_TX_MESSAGE,
    };

    command_session_tx_message tx_session_msg;
    tx_session_msg.session_id = session_id;
    tx_session_msg.msg = std::move(session_msg);
    msg.data.append_range(tx_session_msg.as_buffer());

    send_to_network_thread(std::move(msg));
  }

  void driver::handle_response(message&& msg) {
    CORE_LOG_DEBUG("  - RESPONSE");
    auto itr = std::ranges::find_if(resp_list.pending_responses, [&msg](const response_list::pending_response& response) { return response.header.id == msg.header.id; });

    if (itr != resp_list.pending_responses.end()) {
      CORE_LOG_DEBUG("Response received for message {}", msg.header);
      if (itr->handler.handle_msg != nullptr) {
        CORE_LOG_TRACE("Invoking response callback for message {}", msg.header);
        (this->*itr->handler.handle_msg)(msg.header, msg.data);
      }
      resp_list.pending_responses.erase(itr);
    } else {
      CORE_LOG_ERROR("Received response for unknown message {}", msg.header);
    }
  }

  void driver::on_respond_session_check_in(message_header header, const std::span<const uint8_t> data) {
    session_check_in_response response = other_message_spec::parse<session_check_in_response>(data);
    integer_t session_id = response.session_id;

    auto itr = std::ranges::find_if(app_list.pending_apps, [&](const application_list::other_application& app) { return session_id == app.id; });
    if (itr == app_list.pending_apps.end()) {
      CORE_LOG_ERROR("Server received a session check in for an unknown Other application : {}", session_id);
      return;
    }

    auto [app_itr, success] = app_list.other_apps.insert({ session_id, std::move(*itr) });
    if (!success || app_itr == app_list.other_apps.end()) {
      CORE_LOG_ERROR("Failed to save Other application session ID from check-in : {}", session_id);
      return;
    }
    app_list.pending_apps.erase(itr);
    register_other_application(session_id, &app_itr->second);
  }

  void driver::handle_session_information_response(integer_t session_id, session_information_response&& response) {
    auto itr = app_list.other_apps.find(session_id);
    if (itr == app_list.other_apps.end()) {
      CORE_LOG_ERROR("Cannot process session information response for unknown Other application session ID {}", session_id);
      return;
    }
    application_list::other_application& app = itr->second;

    if (response.project_data_flag || response.name_flag) {
      app.name = response.name;
    }
    if (response.project_data_flag || response.executable_flag) {
      app.executable = filepath(response.executable);
    }
    if (response.project_data_flag || response.working_directory_flag) {
      app.working_directory = filepath(response.working_directory);
    }

    if (!std::filesystem::exists(*app.executable)) {
      CORE_LOG_ERROR("Executable path '{}' for Other application session [{}] does not exist", app.executable->string(), session_id);
      app.executable = {};
    }

    if (!std::filesystem::exists(*app.working_directory)) {
      CORE_LOG_ERROR("Working directory path '{}' for Other application session [{}] does not exist", app.working_directory->string(), session_id);
      app.working_directory = {};
    }

    print_session_information(&app);
  }

  void driver::print_session_information(application_list::other_application* app) {
    CORE_LOG_INFO("Other application session [{}] information:", app->id);
    CORE_LOG_INFO("   - Name: {}", app->get_name());
    CORE_LOG_INFO("   - Executable: {}", app->executable.has_value() ? app->executable->string() : "<none>");
    CORE_LOG_INFO("   - Working Directory: {}", app->working_directory.has_value() ? app->working_directory->string() : "<none>");
  }

  scope<renderer> driver::get_renderer() const {
    if (auto* rendering = subsystem<renderer_backend>::get(); !rendering->has_backend()) {
      rendering->load_backend(configuration(), "opengl", { 1280, 720 });
    }
    return make_scope<renderer>();
  }

  ref<assembly> driver::load_dotnet_module(const std::string_view module_path) {
    PROFILE_SECTION("driver::load_dotnet_module");
    CORE_LOG_DEBUG("Loading script module from path: {}", module_path);
    filepath path(module_path);
    if (!std::filesystem::exists(path)) {
      CORE_LOG_ERROR("Script module path does not exist: {}", module_path);
      return nullptr;
    }
    return subsystem<scripting_environment>::get()->load_dotnet_module(path.string());
  }

  void driver::unload_dotnet_module(ref<assembly> module) {
    if (module == nullptr) {
      CORE_LOG_ERROR("Cannot unload a null module.");
      return;
    }

    CORE_LOG_DEBUG("Unloading script module with ID: {}", module->get_handle());
    subsystem<scripting_environment>::get()->unload_dotnet_module(module);
  }

  void driver::launch_detached_process(const filepath& working_dir, const filepath& exe_name, const std::vector<std::string>& args) {
    launch_process(working_dir, exe_name, args);
  }

  natural_t driver::send_message_and_wait_acknowledgment(message&& msg, microseconds timeout, message_handler handler) {
    acknowledgement_list::pending_ack ack{
      .header = msg.header,
      .timeout_duration = timeout,
      .handler = handler,
      .timer = asio::steady_timer(net_context->io_context),
    };
    CORE_LOG_DEBUG("PENDING-ACK {} (timeout: {} us)", ack.header, ack.timeout_duration.count());

    {
      auto itr = std::find_if(ack_list.pending_acks.begin(), ack_list.pending_acks.end(), [&ack](const acknowledgement_list::pending_ack& existing_ack) {
        return existing_ack.header == ack.header;
      });
      OTHER_ASSERT(itr == ack_list.pending_acks.end(), "Acknowledgment for message ID {} already pending", ack.header);
    }

    ack.sent_time = std::chrono::steady_clock::now();
    ack.id = ack_list.next_pending_ack_id++;
    send_to_network_thread(std::move(msg));
    auto ack_itr = ack_list.pending_acks.insert(ack_list.pending_acks.end(), std::move(ack));
    OTHER_ASSERT(ack_itr != ack_list.pending_acks.end(), "Failed to insert pending acknowledgment for message ID {}", ack.header.id);

    ack_itr->timer.expires_after(timeout);
    ack_itr->timer.async_wait([this, stime = ack.sent_time](const asio::error_code& ec) {
      if (ec && ec == asio::error::operation_aborted) {
        return;
      }
      if (!ec) {
        auto itr = std::ranges::find_if(ack_list.pending_acks, [&](const acknowledgement_list::pending_ack& ack) { return ack.sent_time == stime; });
        if (itr == ack_list.pending_acks.end()) {
          CORE_LOG_ERROR("Failed to find ack for timeout callback!");
        }

        CORE_LOG_WARN("Acknowledgment timeout for message {}", itr->header);
        if (itr->handler.on_timeout) {
          (this->*itr->handler.on_timeout)(itr->header);
        }
      }

      // remove from pending acks
      auto itr = std::ranges::find_if(ack_list.pending_acks, [&](const acknowledgement_list::pending_ack& ack) { return ack.sent_time == stime; });
      if (itr != ack_list.pending_acks.end()) {
        CORE_LOG_DEBUG("Removing pending acknowledgment for message ID {}", itr->header.id);
        ack_list.pending_acks.erase(itr);
      }
    });

    return ack_itr->id;
  }

  void driver::cancel_acknowledgment(natural_t ack_id) {
    auto itr = std::ranges::find_if(ack_list.pending_acks, [&](const acknowledgement_list::pending_ack& ack) { return ack.id == ack_id; });
    if (itr != ack_list.pending_acks.end()) {
      CORE_LOG_DEBUG("Cancelling pending acknowledgment for message ID {}", itr->header.id);
      itr->timer.cancel();
      ack_list.pending_acks.erase(itr);
    }
  }

  void driver::send_message_and_detach_response(message&& msg, message_handler handler) {
    response_list::pending_response response{
      .header = msg.header,
      .sent_time = std::chrono::steady_clock::now(),
      .handler = handler,
      .timer = asio::steady_timer(net_context->io_context),
    };

    {
      auto itr = std::find_if(resp_list.pending_responses.begin(), resp_list.pending_responses.end(), [&response](const response_list::pending_response& existing_response) {
        return existing_response.header == response.header;
      });
      OTHER_ASSERT(itr == resp_list.pending_responses.end(), "Response for message ID {} already pending", response.header.id);
    }

    send_to_network_thread(std::move(msg));
    if (handler.handle_msg == nullptr) {
      return;
    }

    auto resp_itr = resp_list.pending_responses.insert(resp_list.pending_responses.end(), std::move(response));
    OTHER_ASSERT(resp_itr != resp_list.pending_responses.end(), "Failed to insert pending response for message ID {}", response.header.id);
  }

  natural_t driver::send_message_and_wait_response(message&& msg, microseconds timeout, message_handler handler) {
    response_list::pending_response response{
      .header = msg.header,
      .sent_time = std::chrono::steady_clock::now(),
      .handler = handler,
      .timer = asio::steady_timer(net_context->io_context),
    };

    {
      auto itr = std::find_if(resp_list.pending_responses.begin(), resp_list.pending_responses.end(), [&response](const response_list::pending_response& existing_response) {
        return existing_response.header == response.header;
      });
      OTHER_ASSERT(itr == resp_list.pending_responses.end(), "Response for message ID {} already pending", response.header.id);
    }

    send_to_network_thread(std::move(msg));

    response.id = resp_list.next_pending_response_id++;
    auto resp_itr = resp_list.pending_responses.insert(resp_list.pending_responses.end(), std::move(response));
    OTHER_ASSERT(resp_itr != resp_list.pending_responses.end(), "Failed to insert pending response for message ID {}", response.header.id);

    // set up timeout
    resp_itr->timer.expires_after(timeout);
    resp_itr->timer.async_wait([this, stime = resp_itr->sent_time](const asio::error_code& ec) {
      if (ec) {
        return;
      }

      auto itr = std::ranges::find_if(resp_list.pending_responses, [&](const response_list::pending_response& resp) { return resp.sent_time == stime; });
      OTHER_ASSERT(itr != resp_list.pending_responses.end(), "Failed to find response for timeout callback!");
      OTHER_ASSERT(itr->handler.on_timeout != nullptr, "Timeout callback is null for message ID {}", itr->header.id);

      CORE_LOG_WARN("Response timeout for message {}", itr->header);
      (this->*itr->handler.on_timeout)(itr->header);

      resp_list.pending_responses.erase(itr);
    });

    return resp_itr->id;
  }

  void driver::cancel_response(natural_t response_id) {
    auto itr = std::ranges::find_if(resp_list.pending_responses, [&](const response_list::pending_response& resp) { return resp.id == response_id; });
    if (itr != resp_list.pending_responses.end()) {
      CORE_LOG_DEBUG("Cancelling pending response for message ID {}", itr->header.id);
      itr->timer.cancel();
      resp_list.pending_responses.erase(itr);
    }
  }

  natural_t driver::set_timeout(microseconds duration, timer_list::timeout::on_timeout timeout_callback) {
    timer_list::timeout new_timeout{
      .id = timeout_list.next_timeout_id++,
      .timer = asio::steady_timer(net_context->io_context),
    };
    new_timeout.timer.expires_after(duration);
    new_timeout.timer.async_wait([this, timeout_id = new_timeout.id, timeout_callback](const asio::error_code& ec) {
      if (ec) {
        return;
      }

      timeout_callback(timeout_id);

      auto itr = std::ranges::find_if(timeout_list.pending_timeouts, [&](const timer_list::timeout& t) { return t.id == timeout_id; });
      if (itr != timeout_list.pending_timeouts.end()) {
        timeout_list.pending_timeouts.erase(itr);
      }
    });
    timeout_list.pending_timeouts.insert(timeout_list.pending_timeouts.end(), std::move(new_timeout));

    return new_timeout.id;
  }

  void driver::clear_timeout(natural_t timeout_id) {
    auto itr = std::ranges::find_if(timeout_list.pending_timeouts, [&](const timer_list::timeout& t) { return t.id == timeout_id; });
    if (itr != timeout_list.pending_timeouts.end()) {
      itr->timer.cancel();
      timeout_list.pending_timeouts.erase(itr);
    }
  }

  void driver::register_other_application(integer_t session_id, application_list::other_application* app) {
    OTHER_ASSERT(app != nullptr, "Application pointer is null after insertion!");

    app->connected = true;
    CORE_LOG_INFO("Other application [{}] has connected", session_id);
    session_application_information_request(session_id);
  }

  void driver::process_network_thread_messages(message&& msg) {
    switch (msg.header.category) {
      case NOTIFICATION:
        switch (msg.header.id) {
          case STREAM_RX_UDP_DATAGRAM: handle_notification_stream_receive_udp_datagram(std::move(msg)); break;
          case SESSION_CHECK_IN: handle_notification_session_check_in(std::move(msg)); break;
          case SESSION_CLOSED: handle_notification_session_closed(std::move(msg)); break;
          default:
            CORE_LOG_ERROR("Server received unknown notification message ID {}", msg.header.id);
            break;
        }
        break;

      case ACKNOWLEDGEMENT:
        switch (msg.header.id) {
          case ACK: handle_acknowledgement_ack(std::move(msg)); break;
          default: CORE_LOG_ERROR("Driver received unknown acknowledgment message ID {}", msg.header.id); break;
        }
        break;

      case CONTROL:
        switch (msg.header.id) {
          case PING: handle_control_ping(std::move(msg)); break;
          case PONG: handle_control_pong(std::move(msg)); break;
          default:
            CORE_LOG_ERROR("Server received unknown CONTROL message ID {}", msg.header.id);
            break;
        }
        break;

      case COMMAND:
        switch (msg.header.id) {
          default:
            CORE_LOG_ERROR("Server received unknown COMMAND message ID {}", msg.header.id);
            break;
        }
        break;

      case REQUEST:
        switch (msg.header.id) {
          default:
            CORE_LOG_ERROR("Server received unknown REQUEST message ID {}", msg.header.id);
            break;
        }
        break;

      case RESPONSE: handle_response(std::move(msg)); break;

      case SESSION_EVENT:
        switch (msg.header.id) {
          case SESSION_RX_MESSAGE: handle_session_event_rx_message(std::move(msg)); break;
          default:
            CORE_LOG_ERROR("Server received unknown SESSION_EVENT message ID {}", msg.header.id);
            break;
        }
        break;

      case ERROR_ALERT: handle_error_alert(std::move(msg)); break;

      default:
        CORE_LOG_ERROR("Server received unknown message category {}", msg.header.category);
        break;
    }
  }

  void driver::send_to_network_thread(message&& msg) {
    net_context->net_thread_message_bus.send_message(std::move(msg));
  }

  void driver::handle_load_empty_scene_event(const value& data) {
    if (data.type() != value_type::STRING) {
      CORE_LOG_ERROR("Invalid data type for force-load-empty-scene event. Expected string.");
      return;
    }

    std::string scene_name = data.as_string();
    if (project_scene_graph->has_scene(scene_name)) {
      CORE_LOG_WARN("Scene with name [{}] already exists in the scene graph. Cannot force load empty scene with duplicate name.", scene_name);
      return;
    }

    natural_t scene_id = create_empty_scene(scene_name);
    set_scene_to_active(scene_id);
    OTHER_ASSERT(active_scene != nullptr, "Active scene is null after creating/loading scene.");

    CORE_LOG_INFO("Created and loaded empty scene [{}:{}] from console command.", scene_id, scene_name);
    constexpr bool is_empty = true;
    constexpr bool requires_udp_binding = true;
    send_load_command(active_scene->name, scene_id, is_empty, requires_udp_binding);
  }

  void driver::handle_load_scene_event(const value& data) {
    if (data.type() != value_type::STRING) {
      CORE_LOG_ERROR("Invalid data type for load-scene event. Expected string.");
      return;
    }

    std::string scene_path_str = data.as_string();
    filepath scene_path(scene_path_str);
    if (!std::filesystem::exists(scene_path)) {
      CORE_LOG_ERROR("Scene file '{}' does not exist. Cannot load scene.", scene_path.string());
      return;
    }

    natural_t scene_id = add_scene_to_scene_graph(scene_path);
    if (scene_id == 0) {
      CORE_LOG_ERROR("Failed to load scene from file '{}' via console command.", scene_path.string());
      return;
    }

    set_scene_to_active(scene_id);
    OTHER_ASSERT(active_scene != nullptr, "Active scene is null after loading scene.");

    constexpr bool is_empty = false;
    constexpr bool requires_udp_binding = true;
    send_load_command(active_scene->name, scene_id, is_empty, requires_udp_binding);

    if (rendering_enabled()) {
      render_data data = active_scene->prepare_render_data(asset_mgr);
      if (data.primary_camera == nullptr) {
        CORE_LOG_WARN("Loaded scene '{}' does not have a primary camera set. Creating [Camera] Entity.", active_scene->name);
        scene_object& obj = active_scene->create_object("Camera");
        active_scene->add_component<camera_component>(obj.id);
        active_scene->add_object_tag(obj.id, "main-camera");
      }
    }
  }

  void driver::send_load_command(const std::string_view scene_name, natural_t scene_id, bool is_empty, bool requires_udp_binding) {
    message cmd_msg;
    cmd_msg.header = {
      .category = COMMAND,
      .id = ENVIRONMENT_LOAD_SCENE,
    };

    command_load_scene scene_cmd;
    scene_cmd.session_id_flag = client_session_id.has_value() ? 0x01 : 0x00;
    if (client_session_id.has_value()) {
      scene_cmd.session_id = client_session_id.value();
    }

    scene_cmd.empty_scene_flag = is_empty ? 0x01 : 0x00;
    scene_cmd.requires_udp_binding = requires_udp_binding ? 0x01 : 0x00;
    if (requires_udp_binding) {
      scene_cmd.udp_address = { network_context::kLocalhostAddress, net_context->next_available_server_port++ };
      scene_cmd.server_udp_address = { network_context::kLocalhostAddress, net_context->next_available_server_port++ };
    }

    scene_cmd.scene_name = active_scene->name;
    cmd_msg.data.append_range(scene_cmd.as_buffer());

    CORE_LOG_DEBUG("Sending command to network thread to load empty scene '{}'", scene_name);
    if (scene_cmd.requires_udp_binding == 0x01) {
      CORE_LOG_DEBUG("Scene '{}' requires UDP binding @ [LOCAL = {}, REMOTE = {}]", scene_name, binding_point::write_string(scene_cmd.udp_address), binding_point::write_string(scene_cmd.server_udp_address));
    }

    /// \todo check if server is even open
    CORE_LOG_INFO("sending ENVIRONMENT_LOAD_SCENE command for remote....");
    send_message_and_wait_acknowledgment(std::move(cmd_msg), seconds(10), message_handler{ this, &driver::on_acknowledge_command_environment_load_scene, &driver::on_timeout_environment_load_scene });
  }

  void driver::handle_open_ui_window_event(const value& data) {
    if (data.type() == value_type::STRING) {
      std::string window_type_str = data;
      if (window_type_str == "console") {
        open_ui_window(driver_ui::BUILTIN_WINDOW_CONSOLE);
      } else if (window_type_str == "viewport") {
        open_ui_window(driver_ui::BUILTIN_WINDOW_VIEWPORT);
      } else if (window_type_str == "type-database") {
        open_ui_window(driver_ui::BUILTIN_WINDOW_TYPE_DATABASE);
      } else {
        CORE_LOG_ERROR("Unknown UI window type requested to open: {}", window_type_str);
      }
    } else if (data.type() == value_type::INT32) {
      int32_t window_type_int = data;
      if (window_type_int >= 0 && window_type_int < static_cast<int32_t>(driver_ui::NUM_BUILTIN_WINDOW_TYPES)) {
        open_ui_window(static_cast<driver_ui::builtin_window_type>(window_type_int));
      } else {
        CORE_LOG_ERROR("Invalid UI window type index requested to open: {}", window_type_int);
      }
    } else {
      CORE_LOG_ERROR("Invalid data type for open-driver-ui-window event: {}", data.type());
      return;
    }
  }

  void driver::handle_close_ui_window_event(const value& data) {
    if (data.type() == value_type::STRING) {
      std::string window_type_str = data;
      if (window_type_str == "console") {
        close_ui_window(driver_ui::BUILTIN_WINDOW_CONSOLE);
      } else if (window_type_str == "viewport") {
        close_ui_window(driver_ui::BUILTIN_WINDOW_VIEWPORT);
      } else if (window_type_str == "type-database") {
        close_ui_window(driver_ui::BUILTIN_WINDOW_TYPE_DATABASE);
      } else {
        CORE_LOG_ERROR("Unknown UI window type requested to close: {}", window_type_str);
      }
    } else if (data.type() == value_type::INT32) {
      int32_t window_type_int = data;
      if (window_type_int >= 0 && window_type_int < static_cast<int32_t>(driver_ui::NUM_BUILTIN_WINDOW_TYPES)) {
        close_ui_window(static_cast<driver_ui::builtin_window_type>(window_type_int));
      } else {
        CORE_LOG_ERROR("Invalid UI window type index requested to close: {}", window_type_int);
      }
    } else {
      CORE_LOG_ERROR("Invalid data type for close-driver-ui-window event: {}", data.type());
      return;
    }
  }

  natural_t driver::add_scene_to_scene_graph(const filepath& scene_path) {
    OTHER_ASSERT(project_scene_graph != nullptr, "Project scene graph is not initialized.");
    auto [id, scene_ptr] = project_scene_graph->load_scene(scene_path);
    if (scene_ptr == nullptr) {
      return 0;
    }
    return id;
  }

  natural_t driver::create_empty_scene(const std::string_view name) {
    auto [id, _] = project_scene_graph->create_new_scene(name);
    return id;
  }

  natural_t driver::get_id_of_scene(const std::string_view name) {
    return project_scene_graph->get_id_of_scene(name);
  }

  void driver::driver::add_live_coroutine(task handle) {
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
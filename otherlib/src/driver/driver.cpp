/**
 * \file driver/driver.cpp
 **/
#include "driver/driver.hpp"

#include <SDL3/SDL.h>
#include <SDL3/SDL_events.h>

#include "core/defines.hpp"
#include "core/logger.hpp"
#include "serialization/serialization.hpp"
#include "thread/message.hpp"

#include "renderer/renderer_backend.hpp"
#include "script/scripting_environment.hpp"

#include "scripting/lua_bindings.hpp"
#include "vm/control_table.hpp"
#include "vm/other_device.hpp"
#include "vm/vm.hpp"

namespace other {

  void driver::initialize(const command_line& cmd) {
    PROFILE_SECTION("driver::initialize");

    /// set signal catchers
    net_context = std::make_unique<network_context>();
    net_context->signals.async_wait([this](std::error_code ec, int signum) {
      if (!ec) {
        catch_signal(signum);
      } else {
        CORE_LOG_ERROR("Error while waiting for signal: {}", ec.message());
      }
    });

    net_context->events = make_scope<event_system>(net_context->io_context);
    net_context->events->register_event("shutdown-requested");

    net_context->net_thread = make_scope<network_thread>(net_context->net_thread_message_bus);
    net_context->net_thread->launch();
    net_context->net_thread_message_bus.register_thread();

    std::vector<std::string> dotnet_modules = get_config_value<std::vector<std::string>>("scripting", "dotnet-modules");
    for (const auto& module : dotnet_modules) {
      CORE_LOG_DEBUG(" - .NET module to load: {}", module);
      auto assembly = load_dotnet_module(module);
      if (assembly == nullptr) {
        CORE_LOG_ERROR("Failed to load .NET module: {}", module);
      }

      loaded_dotnet_modules.push_back(assembly);
    }

    vm::initialize_device(&core_device);
    vm::activate_builtin_control_table(&core_device, OTHER_CONTROL_TABLE_V000);
    core_device.stopped = false;
    core_device.host_driver = this;

    project_scene_graph = make_scope<scene_graph>();

    auto* env = subsystem<scripting_environment>::get();
    bind_otherlib_driver_lua_functions(env->get_lua_host(), this);
    {
      PROFILE_SECTION("driver::initialize--client-on_initialize");
      on_initialize(cmd);
    }
  }

  void driver::shutdown() {
    PROFILE_SECTION("driver::shutdown");
    {
      PROFILE_SECTION("driver::shutdown--client-on_shutdown");
      on_shutdown();
    }

    core_device.stopped = true;
    vm::shutdown_device(&core_device);

    for (auto& module : loaded_dotnet_modules) {
      unload_dotnet_module(module);
    }
    loaded_dotnet_modules.clear();

    project_scene_graph = nullptr;

    net_context->net_thread = nullptr;

    net_context->events = nullptr;
    net_context->signals.cancel();
    if (!net_context->io_context.stopped()) {
      net_context->io_context.stop();
    }
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

  scene* driver::get_active_scene() {
    return active_scene;
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

  void driver::set_scene_to_active(natural_t scene_id) {
    CORE_LOG_DEBUG("Setting scene [{}] as active scene in driver.", scene_id);
    active_scene = project_scene_graph->get_scene(scene_id);
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

  void driver::pump_events() {
    PROFILE_SECTION("driver::pump_events");

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      switch (event.type) {
        case SDL_EVENT_WINDOW_CLOSE_REQUESTED: {
          if (event.window.windowID == SDL_GetWindowID(subsystem<renderer_backend>::get()->get_main_window())) {
            shutdown_requested = true;
          }
        } break;

        default: {
        } break;
      }

      subsystem<renderer_backend>::get()->handle_event(&event);
      on_event(&event);
    }

    poll_coroutines();

    net_context->io_context.poll();
    if (net_context->io_context.stopped()) {
      net_context->io_context.restart();
    }

    auto msg_opt = net_context->net_thread_message_bus.receive_message();
    if (msg_opt.has_value()) {
      process_network_thread_messages(std::move(*msg_opt));
    }
  }

  void driver::handle_session_event_rx_message(message&& msg) {
    auto bytes = std::span(msg.data);

    integer_t session_id = *reinterpret_cast<integer_t*>(bytes.data());
    bytes = bytes.subspan(sizeof(integer_t));

    message_header msg_header = *reinterpret_cast<message_header*>(bytes.data());
    bytes = bytes.subspan(sizeof(message_header));

    message rx_msg;
    rx_msg.header = msg_header;
    rx_msg.data.append_range(std::span(bytes));

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

    const uint8_t* session_id_bytes = reinterpret_cast<const uint8_t*>(&session_id);
    const uint8_t* session_msg_header_bytes = reinterpret_cast<const uint8_t*>(&resp_msg.header);
    const uint8_t* session_msg_data_bytes = reinterpret_cast<const uint8_t*>(resp_msg.data.data());
    tx_msg.data.append_range(std::span(session_id_bytes, sizeof(integer_t)));
    tx_msg.data.append_range(std::span(session_msg_header_bytes, sizeof(message_header)));
    tx_msg.data.append_range(std::span(session_msg_data_bytes, resp_msg.data.size()));

    send_message_and_detach_response(std::move(tx_msg), nullptr);
  }

  void driver::handle_response_session_information(integer_t session_id, message&& msg) {
    session_information_response response = other_message_spec::parse<session_information_response>(std::span(msg.data));
    handle_session_information_response(session_id, std::move(response));
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

  natural_t driver::send_message_and_wait_acknowledgment(message&& msg, microseconds timeout, pending_ack::on_ack ack_callback, pending_ack::on_timeout timeout_callback) {
    pending_ack ack{
      .header = msg.header,
      .timeout_duration = timeout,
      .ack_callback = ack_callback,
      .timeout_callback = timeout_callback,
      .timer = asio::steady_timer(net_context->io_context),
    };
    CORE_LOG_DEBUG("PENDING-ACK {} (timeout: {} us)", ack.header, ack.timeout_duration.count());

    {
      auto itr = std::find_if(pending_acks.begin(), pending_acks.end(), [&ack](const pending_ack& existing_ack) {
        return existing_ack.header == ack.header;
      });
      OTHER_ASSERT(itr == pending_acks.end(), "Acknowledgment for message ID {} already pending", ack.header);
    }

    ack.sent_time = std::chrono::steady_clock::now();
    ack.id = next_pending_ack_id++;
    net_context->net_thread_message_bus.send_message(std::move(msg));
    auto ack_itr = pending_acks.insert(pending_acks.end(), std::move(ack));
    OTHER_ASSERT(ack_itr != pending_acks.end(), "Failed to insert pending acknowledgment for message ID {}", ack.header.id);

    ack_itr->timer.expires_after(timeout);
    ack_itr->timer.async_wait([this, stime = ack.sent_time](const asio::error_code& ec) {
      if (!ec) {
        auto itr = std::ranges::find_if(pending_acks, [&](const pending_ack& ack) { return ack.sent_time == stime; });
        if (itr == pending_acks.end()) {
          CORE_LOG_ERROR("Failed to find ack for timeout callback!");
        }

        CORE_LOG_WARN("Acknowledgment timeout for message {}", itr->header);
        if (itr->timeout_callback) {
          itr->timeout_callback(itr->header);
        }
      }

      // remove from pending acks
      auto itr = std::ranges::find_if(pending_acks, [&](const pending_ack& ack) { return ack.sent_time == stime; });
      if (itr != pending_acks.end()) {
        CORE_LOG_DEBUG("Removing pending acknowledgment for message ID {}", itr->header.id);
        pending_acks.erase(itr);
      }
    });

    return ack_itr->id;
  }

  void driver::cancel_acknowledgment(natural_t ack_id) {
    auto itr = std::ranges::find_if(pending_acks, [&](const pending_ack& ack) { return ack.id == ack_id; });
    if (itr != pending_acks.end()) {
      CORE_LOG_DEBUG("Cancelling pending acknowledgment for message ID {}", itr->header.id);
      itr->timer.cancel();
      pending_acks.erase(itr);
    }
  }

  void driver::send_message_and_detach_response(message&& msg, pending_response::on_response response_callback) {
    pending_response response{
      .header = msg.header,
      .sent_time = std::chrono::steady_clock::now(),
      .response_callback = response_callback,
      .timeout_callback = nullptr,
      .timer = asio::steady_timer(net_context->io_context),
    };

    {
      auto itr = std::find_if(pending_responses.begin(), pending_responses.end(), [&response](const pending_response& existing_response) {
        return existing_response.header == response.header;
      });
      OTHER_ASSERT(itr == pending_responses.end(), "Response for message ID {} already pending", response.header.id);
    }

    net_context->net_thread_message_bus.send_message(std::move(msg));
    if (response_callback == nullptr) {
      return;
    }

    auto resp_itr = pending_responses.insert(pending_responses.end(), std::move(response));
    OTHER_ASSERT(resp_itr != pending_responses.end(), "Failed to insert pending response for message ID {}", response.header.id);
  }

  natural_t driver::send_message_and_wait_response(message&& msg, microseconds timeout, pending_response::on_response response_callback, pending_response::on_timeout timeout_callback) {
    pending_response response{
      .header = msg.header,
      .sent_time = std::chrono::steady_clock::now(),
      .response_callback = response_callback,
      .timeout_callback = timeout_callback,
      .timer = asio::steady_timer(net_context->io_context),
    };

    {
      auto itr = std::find_if(pending_responses.begin(), pending_responses.end(), [&response](const pending_response& existing_response) {
        return existing_response.header == response.header;
      });
      OTHER_ASSERT(itr == pending_responses.end(), "Response for message ID {} already pending", response.header.id);
    }

    net_context->net_thread_message_bus.send_message(std::move(msg));

    response.id = next_pending_response_id++;
    auto resp_itr = pending_responses.insert(pending_responses.end(), std::move(response));
    OTHER_ASSERT(resp_itr != pending_responses.end(), "Failed to insert pending response for message ID {}", response.header.id);

    // set up timeout
    resp_itr->timer.expires_after(timeout);
    resp_itr->timer.async_wait([this, stime = resp_itr->sent_time](const asio::error_code& ec) {
      if (ec) {
        return;
      }

      auto itr = std::ranges::find_if(pending_responses, [&](const pending_response& resp) { return resp.sent_time == stime; });
      OTHER_ASSERT(itr != pending_responses.end(), "Failed to find response for timeout callback!");
      OTHER_ASSERT(itr->timeout_callback != nullptr, "Timeout callback is null for message ID {}", itr->header.id);

      CORE_LOG_WARN("Response timeout for message {}", itr->header);
      itr->timeout_callback(itr->header);

      pending_responses.erase(itr);
    });

    return resp_itr->id;
  }

  void driver::cancel_response(natural_t response_id) {
    auto itr = std::ranges::find_if(pending_responses, [&](const pending_response& resp) { return resp.id == response_id; });
    if (itr != pending_responses.end()) {
      CORE_LOG_DEBUG("Cancelling pending response for message ID {}", itr->header.id);
      itr->timer.cancel();
      pending_responses.erase(itr);
    }
  }

  natural_t driver::set_timeout(microseconds duration, timeout::on_timeout timeout_callback) {
    timeout new_timeout{
      .id = next_timeout_id++,
      .timer = asio::steady_timer(net_context->io_context),
    };
    new_timeout.timer.expires_after(duration);
    new_timeout.timer.async_wait([this, timeout_id = new_timeout.id, timeout_callback](const asio::error_code& ec) {
      if (ec) {
        return;
      }

      timeout_callback(timeout_id);

      auto itr = std::ranges::find_if(pending_timeouts, [&](const timeout& t) { return t.id == timeout_id; });
      if (itr != pending_timeouts.end()) {
        pending_timeouts.erase(itr);
      }
    });
    pending_timeouts.insert(pending_timeouts.end(), std::move(new_timeout));

    return new_timeout.id;
  }

  void driver::clear_timeout(natural_t timeout_id) {
    auto itr = std::ranges::find_if(pending_timeouts, [&](const timeout& t) { return t.id == timeout_id; });
    if (itr != pending_timeouts.end()) {
      itr->timer.cancel();
      pending_timeouts.erase(itr);
    }
  }

  void driver::process_network_thread_messages(message&& msg) {
    switch (msg.header.category) {
      case NOTIFICATION:
        switch (msg.header.id) {
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

  natural_t driver::add_scene_to_scene_graph(const filepath& scene_path) {
    // OTHER_ASSERT(project_scene_graph != nullptr, "Project scene graph is not initialized.");
    // return project_scene_graph->add_scene_from_file(scene_path);
    CORE_LOG_ERROR("driver::add_scene_to_scene_graph is unimplemented.");
    return 0;
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
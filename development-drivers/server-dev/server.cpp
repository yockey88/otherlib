/**
 * \file server-dev/server.cpp
 **/
#include "server.hpp"

#include <filesystem>

#include "core/coroutine.hpp"
#include "core/defines.hpp"
#include "serialization/serialization.hpp"
#include "thread/message.hpp"
#include "thread/messages.hpp"

#include "renderer/ui/ui_helpers.hpp"
#include "script/scripting_environment.hpp"

#include "driver/driver.hpp"
#include "rendering-pipelines/empty_pipeline.hpp"
#include "tools/build_tool.hpp"

#include "server_tasks.hpp"

#include "server-ui/project-creator.hpp"

namespace other {

  void server::on_initialize(const command_line& cmd) {
    state_machine.handle_event(server_event::SERVER_EVENT_START, this);
    filepath app_folder = get_project_cache();

    std::ifstream file(app_folder);
    if (file.is_open()) {
      file >> project_cache;
      file.close();
    } else {
      CORE_LOG_WARN("Failed to open project cache file at {}", app_folder.string());
    }

    /// create event system
    get_event_system()->register_event("open-project");
    get_event_system()->add_listener("open-project", [this](const value& data) {
      /// \todo open project
      CORE_LOG_DEBUG("Received request to open project");
      std::string name = data;

      filepath project_path;
      filepath working_dir;

      auto projects = project_cache["projects"];
      json::json project_entry;
      for (const auto& p : projects.items()) {
        if (p.value().contains("name") && p.value()["name"].get<std::string>() == name) {
          project_entry = p.value();
          break;
        }
      }

      if (project_entry.is_null()) {
        CORE_LOG_ERROR("Project '{}' not found in project cache", name);
        return;
      }

      std::string file = project_entry.contains("project-file") ? project_entry["project-file"].get<std::string>() : "";

      CORE_LOG_DEBUG("Opening project '{}' at path '{}' with working directory '{}'", name, file, project_entry.at("working-directory").get<std::string>());
      validate_project_and_launch(project_entry);
    });

    get_event_system()->register_event("finalize-project");
    get_event_system()->add_listener("finalize-project", [this](const value& data) {
      post_coroutine(build_project(data, project_cache, *get_event_system()));
    });

    {
      message msg;
      msg.header = {
        .category = COMMAND,
        .id = SESSION_LISTEN_FOR,
      };
      uint16_t port = 49222;
      const uint8_t* port_bytes = reinterpret_cast<const uint8_t*>(&port);
      msg.data.append_range(std::span(port_bytes, sizeof(uint16_t)));

      send_message_and_wait_acknowledgment(
        std::move(msg), std::chrono::seconds(10),
        std::bind_front(&server::on_ack_session_listen_for_network_thread, this),
        std::bind_front(&server::on_timeout_session_listen_for_network_thread, this)
      );
    }

    if (rendering_enabled()) {
      renderer = get_renderer();
      renderer->add_pipeline<empty_pipeline>("UI Pipeline");

      ui_ptr = make_scope<server_ui>(renderer, get_event_system(), project_cache);
    }

    running = true;
  }

  void server::run() {
    while (running) {
      core_update();

      switch (state_machine.get_current_state()) {
        case server_state::SERVER_STATE_INITIALIZING: update_initializing(); break;
        case server_state::SERVER_STATE_RUNNING: update_running(); break;
        case server_state::SERVER_STATE_SHUTTING_DOWN: update_shutting_down(); break;
        case server_state::SERVER_STATE_SHUT_DOWN: update_shut_down(); break;
        default:
          CORE_LOG_ERROR("Server in unknown state {}", state_machine.get_current_state());
          running = false;
          break;
      }

      if (rendering_enabled()) {
        renderer->begin_frame(nullptr);
        renderer->render();
        ui_ptr->render();
        renderer->end_frame();
      }
    }
  }

  void server::on_shutdown() {
    if (rendering_enabled()) {
      ui_ptr = nullptr;
      renderer->remove_pipeline("UI Pipeline");
      renderer = nullptr;
    }
    CORE_LOG_INFO("Server shutdown complete.");
  }

  void server::catch_signal(int signum) {
    if (signum == SIGINT || signum == SIGTERM) {
      CORE_LOG_INFO("Received signal {}, shutting down server...", signum);
      on_shutdown_request();
    } else {
      CORE_LOG_WARN("Received unhandled signal {}", signum);
    }
  }

  std::string server::other_application::get_name() const {
    return name.has_value() ? *name : (executable.has_value() ? executable->filename().stem().string() : "<unnamed>");
  }

  void server::core_update() {
    pump_events();
  }

  void server::update_initializing() {}

  void server::update_running() {}

  void server::update_shutting_down() {}

  void server::update_shut_down() {
    running = false;
    CORE_LOG_DEBUG("Server shut down complete");
  }

  void server::on_event(SDL_Event* event) {
    OTHER_ASSERT(event != nullptr, "Event is null");
    switch (event->type) {
      case SDL_EVENT_WINDOW_CLOSE_REQUESTED: on_shutdown_request(); break;
      default: break;
    }
  }

  void server::validate_project_and_launch(const json::json& project_entry) {
    std::string name = project_entry.at("name").get<std::string>();

    json::json launch_info = project_entry.at("build");
    std::string type = launch_info.at("type").get<std::string>();

    enum class launch_type {
      OTHER_APPLICATION_EXE,
      UNKNOWN,
    };

    launch_type ltype = launch_type::UNKNOWN;
    if (type == "other-application") {
      ltype = launch_type::OTHER_APPLICATION_EXE;
    } else {
      CORE_LOG_ERROR("Unknown launch type '{}' for project '{}'", type, name);
      return;
    }

    switch (ltype) {
      case launch_type::OTHER_APPLICATION_EXE: {
        begin_other_application(project_entry);
      } break;

      case launch_type::UNKNOWN:
      default:
        CORE_LOG_ERROR("Unhandled launch type for project '{}'", name);
        break;
    }
  }

  std::string replace_all_substrings_with(std::string str, const std::string& from, const std::string& to) {
    size_t start_pos = 0;
    while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
      str.replace(start_pos, from.length(), to);
      start_pos += to.length();  // Move past the replaced substring to avoid infinite loops if 'to' contains 'from'
    }
    return str;
  }

  void server::begin_other_application(const json::json& project_entry) {
    /// launch other application with command line args specifying the server's current command port, the project configuration
    ///   and the working directory
    /// set an event listener for 'session-initial-check-in' to register other-application with the server
    /// respond to the ping with server data

#if 0
  #define TESTING_COROS
#endif
    // post_coroutine(validate_and_build_other_application("OtherApp", working_dir, exe_name));
#ifdef TESTING_COROS
#else

    std::string name = project_entry.at("name").get<std::string>();
    filepath project_file = filepath(project_entry.at("project-file").get<std::string>());
    filepath working_dir = project_entry.at("working-directory").get<std::string>();
    json::json build_info = project_entry.at("build");

    opt<filepath> output_file = build_info.contains("output-file") ? filepath(build_info.at("output-file").get<std::string>()) : opt<filepath>{};
    filepath exe_name = build_info.contains("executable") ? filepath(build_info.at("executable").get<std::string>()) : filepath("OtherApp.exe");
    std::vector<std::string> args = build_info.contains("args") ? build_info.at("args").get<std::vector<std::string> >() : std::vector<std::string>{};

    static integer_t next_id = 1;
    integer_t id = next_id++;
    auto itr = pending_apps.insert(pending_apps.end(), other_application{ .id = id, .working_directory = working_dir, .executable = exe_name, .args = args });
    OTHER_ASSERT(itr != pending_apps.end(), "Failed to begin Other application : {}  [{}]", id, exe_name.string());

    CORE_LOG_DEBUG("Starting Other application : {}", id);
    itr->executable = replace_all_substrings_with(itr->executable->string(), "${configuration}", "Debug");

    CORE_LOG_DEBUG("Launching Other application executable '{}' @ [{}]:", itr->executable->string(), working_dir.string());
    for (const auto& arg : itr->args) {
      CORE_LOG_DEBUG("   - {}", arg);
    }

    /// \todo: build the project and validate it is correct first
    // project_description proj_desc = {
    //   .project_type = project_description::APPLICATION,
    //   .name = name,
    //   .working_directory = working_dir,
    //   .output_directory = output_file.has_value() ? output_file->parent_path() : working_dir / filepath("build"),
    //   .exe_name = output_file.has_value() ? *output_file : working_dir / filepath("build") / exe_name,
    //   .configurations = { "Debug", "Release" },
    //   .active_configuration = 0,
    //   .cmd_args = args,
    //   .version = "0.1.0",
    //   .description = "An Other application.",
    //   .author = "Author Name",
    //   .license = "MIT",
    // };
    // if (project_entry.contains("project-file")) {
    //   proj_desc.override_file_name = project_entry.at("project-file").get<std::string>();
    // }
    // build_tool bt;
    // bt.start_build(proj_desc);

    {
      message msg;
      msg.header = {
        .category = REQUEST,
        .id = SESSION_CHECK_IN,
      };

      const uint8_t* id_bytes = reinterpret_cast<const uint8_t*>(&id);
      msg.data.append_range(std::span(id_bytes, sizeof(integer_t)));
      send_message_and_detach_response(std::move(msg), std::bind_front(&server::on_respond_session_check_in_network_thread, this));
    }

    itr->args.insert(itr->args.begin(), project_file.string());
    itr->args.append_range(std::vector<std::string>{ "--sid", std::to_string(itr->id) });
    itr->args.append_range(std::vector<std::string>{ "--port", std::to_string(net_context->main_binding_point.port) });
    launch_detached_process(*itr->working_directory, *itr->executable, itr->args);
#endif
  }

  void server::on_ack_session_listen_for_network_thread(message_header header, const std::span<const uint8_t> data) {
    CORE_LOG_INFO("Network thread acknowledged event request at session check in for session [{}]", header.id);
    state_machine.handle_event(server_event::SERVER_EVENT_READY, this);

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
      net_context->netw_thread_heartbeat_timeout_id = set_timeout(milliseconds(250), [this](natural_t timeout_id) {
        CORE_LOG_ERROR("Network thread failed to respond to PING within timeout period");
        /// handle_network_thread_unresponsive();
      });

      net_context->net_thread_message_bus.send_message(std::move(msg));
    });
  }

  void server::on_timeout_session_listen_for_network_thread(message_header header) {
    CORE_LOG_WARN("Network thread timed out waiting for event request at session check in for session [{}]", header.id);
  }

  void server::on_ack_shutdown_request_network_thread(message_header header, const std::span<const uint8_t> data) {
    CORE_LOG_DEBUG("Network thread acknowledged shutdown request");

    net_context->net_thread->shutdown();
    state_machine.handle_event(server_event::SERVER_EVENT_SHUT_DOWN, this);
  }

  void server::on_timeout_shutdown_request_network_thread(message_header header) {
    CORE_LOG_WARN("Network thread timed out waiting for shutdown acknowledgment");

    /// force stop this time
    net_context->io_context.stop();

    static size_t attempts = 0;
    attempts++;
    if (attempts >= 3) {
      CORE_LOG_ERROR("Network thread failed to shutdown after {} attempts, forcing exit", attempts);
      try {
        net_context->net_thread->force_shutdown();
        state_machine.handle_event(server_event::SERVER_EVENT_SHUT_DOWN, this);
        return;
      } catch (const std::exception& e) {
        CORE_LOG_ERROR("Error occurred while shutting down network thread: {}", e.what());
      }
    }

    CORE_LOG_TRACE("Retrying network thread shutdown...");
    on_shutdown_request();
  }

  void server::on_respond_session_check_in_network_thread(message_header header, const std::span<const uint8_t> data) {
    OTHER_ASSERT(data.size() >= sizeof(integer_t), "Invalid session check in packet!");

    integer_t session_id = *reinterpret_cast<const integer_t*>(data.data());
    other_application* app = nullptr;
    {
      auto itr = std::ranges::find_if(pending_apps, [&](const other_application& app) { return session_id == app.id; });
      if (itr == pending_apps.end()) {
        CORE_LOG_ERROR("Server received a session check in for an unknown Other application : {}", session_id);
        return;
      }

      auto [app_itr, success] = other_apps.insert({ session_id, std::move(*itr) });
      if (!success || app_itr == other_apps.end()) {
        CORE_LOG_ERROR("Failed to save Other application session ID from check-in : {}", session_id);
        return;
      }
      pending_apps.erase(itr);
      app = &app_itr->second;
    }
    register_other_application(session_id, app);
  }

  void server::send_session_information_request(integer_t session_id, other_application* app) {
    if (app == nullptr) {
      auto itr = other_apps.find(session_id);
      if (itr == other_apps.end()) {
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

    const uint8_t* session_id_bytes = reinterpret_cast<const uint8_t*>(&session_id);
    const uint8_t* session_msg_header_bytes = reinterpret_cast<const uint8_t*>(&session_msg.header);
    const uint8_t* session_msg_data_bytes = reinterpret_cast<const uint8_t*>(session_msg.data.data());
    msg.data.append_range(std::span(session_id_bytes, sizeof(integer_t)));
    msg.data.append_range(std::span(session_msg_header_bytes, sizeof(message_header)));
    msg.data.append_range(std::span(session_msg_data_bytes, session_msg.data.size()));

    net_context->net_thread_message_bus.send_message(std::move(msg));
  }

  void server::handle_session_information_response(integer_t session_id, session_information_response&& response) {
    auto itr = other_apps.find(session_id);
    if (itr == other_apps.end()) {
      CORE_LOG_ERROR("Cannot process session information response for unknown Other application session ID {}", session_id);
      return;
    }
    other_application& app = itr->second;

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

  void server::on_response_request_session_information(message_header header, const std::span<const uint8_t> data) {
    OTHER_ASSERT(data.size() >= sizeof(integer_t), "Invalid session information response packet!");

    size_t cursor = 0;
    integer_t session_id = serialization::read_value<integer_t>(data, cursor);

    auto itr = other_apps.find(session_id);
    if (itr == other_apps.end()) {
      CORE_LOG_ERROR("Cannot process session information response for unknown Other application session ID {}", session_id);
      return;
    }
    other_application& app = itr->second;
    app.session_info_request_resp_id = 0;

    session_information_response response = other_message_spec::parse<session_information_response>(data);
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

  void server::on_timeout_request_session_information_network_thread(message_header header) {
    CORE_LOG_WARN("Network thread timed out waiting for session information response for session [{}]", header.id);
  }

  void server::print_session_information(other_application* app) {
    CORE_LOG_INFO("Other application session [{}] information:", app->id);
    CORE_LOG_INFO("   - Name: {}", app->get_name());
    CORE_LOG_INFO("   - Executable: {}", app->executable.has_value() ? app->executable->string() : "<none>");
    CORE_LOG_INFO("   - Working Directory: {}", app->working_directory.has_value() ? app->working_directory->string() : "<none>");
  }

  void server::register_other_application(integer_t session_id, other_application* app) {
    OTHER_ASSERT(app != nullptr, "Application pointer is null after insertion!");

    app->connected = true;
    CORE_LOG_INFO("Other application [{}] has connected", session_id);
    send_session_information_request(session_id);
  }

  void server::on_shutdown_request() {
    state_machine.handle_event(server_event::SERVER_EVENT_STOP, this);

    /// clear all pending acks
    for (auto& ack : pending_acks) {
      ack.timer.cancel();
    }
    pending_acks.clear();

    for (auto& response : pending_responses) {
      response.timer.cancel();
    }
    pending_responses.clear();

    CORE_LOG_DEBUG("Sending shutdown request to network thread...");
    {
      message msg;
      msg.header = {
        .category = COMMAND,
        .id = SHUTDOWN_REQUEST,
      };
      const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&msg.header);
      msg.data.append_range(std::span(bytes, sizeof(message_header)));

      send_message_and_wait_acknowledgment(
        std::move(msg), std::chrono::seconds(3),
        std::bind_front(&server::on_ack_shutdown_request_network_thread, this),
        std::bind_front(&server::on_timeout_shutdown_request_network_thread, this)
      );
    }

    get_event_system()->clear();
    if (!net_context->io_context.stopped()) {
      net_context->io_context.stop();
    }
  }

  void server::handle_notification_session_check_in(message&& msg) {
    OTHER_ASSERT(msg.data.size() >= sizeof(integer_t), "Invalid session check-in notification message size");

    integer_t session_id = *reinterpret_cast<const integer_t*>(msg.data.data());
    if (session_id < 1) {
      CORE_LOG_ERROR("Invalid session ID received in check-in notification: {}", session_id);
      return;
    }

    auto itr = other_apps.find(session_id);
    /// this has to be a new connection, bc if we requested it then it would not be a notification
    OTHER_ASSERT(itr == other_apps.end(), "Received session check-in for unknown Other application session ID {}", session_id);

    other_application app = {
      .id = session_id,
      .connected = true,
    };

    auto [app_itr, success] = other_apps.insert({ session_id, std::move(app) });
    if (!success || app_itr == other_apps.end()) {
      CORE_LOG_ERROR("Failed to save Other application session ID from check-in : {}", session_id);
      return;
    }
    register_other_application(session_id, &app_itr->second);
    CORE_LOG_INFO("Session [{}] has checked in.", session_id);
  }

  void server::handle_notification_session_closed(message&& msg) {
    OTHER_ASSERT(msg.data.size() >= sizeof(integer_t), "Invalid session closed notification size");

    integer_t session_id = *reinterpret_cast<const integer_t*>(msg.data.data());
    auto app_itr = other_apps.find(session_id);
    if (app_itr != other_apps.end()) {
      CORE_LOG_INFO("Other application [{}] has disconnected", app_itr->second.get_name());
      app_itr->second.connected = false;
      /// don't remove from list, it might reconnect
      /// \todo set a timeout to remove it after a while if needed
    } else {
      /// ignore because there may be open sessions that aren't other applications
    }
  }

  void server::on_active_scene_udp_handle_bound(udp_handle* handle) {
    post_coroutine(active_scene_udp_loop());
  }

  task server::active_scene_udp_loop() {
    OTHER_ASSERT(get_active_scene() != nullptr, "No active scene to run UDP handle loop on.");
    CORE_LOG_INFO("Starting active scene UDP handle loop...");

    auto now = steady_clock::now();
    auto last = now;
    auto elapsed = now - last;
    while (state_machine.get_current_state() != server_state::SERVER_STATE_SHUT_DOWN && get_active_scene() != nullptr) {
      now = steady_clock::now();
      elapsed += now - last;
      last = now;

      if (elapsed >= seconds(3)) {
        elapsed = steady_clock::duration::zero();

        CORE_LOG_DEBUG("Waiting to receive UDP packet on active scene UDP handle...");
        auto packet = active_scene_udp_handle.receive();
        if (!packet.empty()) {
          CORE_LOG_INFO("Received UDP packet of size {} on active scene UDP handle", packet.size());
          if (packet.size() >= sizeof(message_header) + sizeof(uint16_t)) {
            size_t cursor = 0;
            message_header header = serialization::read_value<message_header>(packet, cursor);
            uint16_t str_len = serialization::read_value<uint16_t>(packet, cursor);

            if (packet.size() >= sizeof(message_header) + sizeof(uint16_t) + str_len) {
              std::string msg_str(reinterpret_cast<const char*>(packet.data() + cursor), str_len);
              CORE_LOG_INFO("UDP Message received (Category: {}, ID: {}): {}", header.category, header.id, msg_str);
            } else {
              CORE_LOG_WARN("Received UDP packet is too small to contain the expected string data");
            }
          } else {
            CORE_LOG_WARN("Received UDP packet is too small to contain a valid message header + size");
          }
        }
      } else {
        co_await task::awaiter{};
      }
    }
  }

  task server::validate_and_build_other_application(const std::string& name, const filepath& folder, const filepath& env_config_path) {
    auto* env = subsystem<scripting_environment>::get();
    OTHER_ASSERT(env != nullptr, "Scripting environment subsystem is not initialized");

    integer_t builder_obj_id = env->create_object("Builder");
    OTHER_ASSERT(builder_obj_id != -1, "Failed to create Builder object in scripting environment");

    env->attach_dotnet_object(builder_obj_id, "Other.BuildTool");
    // co_await task::awaiter{};

    CORE_LOG_DEBUG("Validating and building Other application '{}' in folder '{}'", name, folder.string());
    script_object* builder_obj = env->get_object(builder_obj_id);
    OTHER_ASSERT(builder_obj != nullptr, "Failed to retrieve Builder object from scripting environment");

    builder_obj->dotnet_object->invoke("ValidateAndBuildOtherApplication");  //, name, folder.string(), env_config_path.string());

    env->destroy_object(builder_obj_id);
    co_return;
  }

}  // namespace other
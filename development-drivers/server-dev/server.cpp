/**
 * \file server-dev/server.cpp
 **/
#include "server.hpp"

#include <coroutine>
#include <filesystem>

#include "core/coroutine.hpp"
#include "core/defines.hpp"
#include "serialization/serialization.hpp"
#include "thread/message.hpp"

#include "renderer/ui/ui_helpers.hpp"
#include "script/scripting_environment.hpp"

#include "driver/driver.hpp"
#include "rendering-pipelines/empty_pipeline.hpp"

#include "tools/build_tool.hpp"

#include "server-ui/project-creator.hpp"

namespace other {
  namespace {

    task build_project(const project_creator::project_context& context, json::json& project_cache_path) {
      /// have to copy context since coroutine may outlive project_creator ui-page
      project_creator::project_context ctx = context;

      auto* env = subsystem<scripting_environment>::get();
      OTHER_ASSERT(env != nullptr, "Scripting environment subsystem is not initialized");

      integer_t builder_obj_id = -1;
      builder_obj_id = env->create_object("Builder");

      if (builder_obj_id == -1) {
        CORE_LOG_ERROR("Failed to create Builder object to build project {}", ctx.project_name);
        co_return;
      }

      CORE_LOG_DEBUG("Attaching Builder object to scripting environment to build project '{}'", ctx.project_name);
      env->attach_dotnet_object(builder_obj_id, "Other.BuildTool");
      /// suspend just to let the engine breathe
      CORE_LOG_DEBUG("Began building project '{}'", ctx.project_name);
      co_await task::awaiter{};

      script_object* builder_obj = env->get_object(builder_obj_id);
      if (builder_obj == nullptr) {
        env->destroy_object(builder_obj_id);
        CORE_LOG_ERROR("Failed to retrieve Builder object from scripting environment to build project {}", ctx.project_name);
        co_return;
      }

      struct build_args_ {
        native_string project_type;
        native_string name;
        native_string filename;
        native_string working_directory;
      } args;
      /// \todo make project type selectable
      args.project_type = "Application";
      args.name = context.project_name;
      args.filename = context.project_path.string();
      args.working_directory = context.working_directory.string();

      CORE_LOG_DEBUG("Invoking CreateProject on Builder object for project '{}'", context.project_name);
      builder_obj->dotnet_object->invoke<void>("CreateProject", args);
      // builder_obj->dotnet_object->invoke<void>("BuildProject", args);

      env->detach_dotnet_object(builder_obj_id);
      env->destroy_object(builder_obj_id);
      co_return;
    }

  }  // namespace

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
    events = make_scope<event_system>(net_context->io_context);
    events->register_event("open-project");
    events->add_listener("open-project", [this](const value& data) {
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

    events->register_event("finalize-project");
    events->add_listener("finalize-project", [this](const value& data) {
      post_coroutine(build_project(data, project_cache));
    });

    /// launch threads
    ///  - networking thread
    net_thread = make_scope<network_thread>(net_thread_message_bus);
    net_thread->launch();
    net_thread_message_bus.register_thread();

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

      ui_ptr = make_scope<server_ui>(renderer, events, project_cache);
    }
    active_scene = scene("Server-Scene");

    running = true;
  }

  void server::run() {
    while (running) {
      pump_events();
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
        render_data data = active_scene.prepare_render_data();
        renderer->begin_frame(&data);
        renderer->render();
        ui_ptr->render();
        renderer->end_frame();
      }
    }
  }

  void server::on_shutdown() {
    events = nullptr;

    if (rendering_enabled()) {
      ui_ptr = nullptr;
      renderer->remove_pipeline("UI Pipeline");
      renderer = nullptr;
    }

    net_thread = nullptr;
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

  filepath server::get_project_cache() {
    filepath cache_file = get_app_data_folder("OtherEngine/OtherServer") / filepath("project_cache.json");
    if (!std::filesystem::exists(cache_file)) {
      std::ofstream file(cache_file);
      file << "{}";
      file.close();
    }
    return cache_file;
  }

  void server::core_update() {
    net_context->io_context.poll();
    if (net_context->io_context.stopped()) {
      net_context->io_context.restart();
    }

    auto msg_opt = net_thread_message_bus.receive_message();
    if (msg_opt.has_value()) {
      process_network_thread_messages(std::move(*msg_opt));
    }
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

  void server::send_message_and_wait_acknowledgment(message&& msg, microseconds timeout, pending_ack::on_ack ack_callback, pending_ack::on_timeout timeout_callback) {
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
    net_thread_message_bus.send_message(std::move(msg));
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
  }

  void server::send_message_and_detach_response(message&& msg, pending_response::on_response response_callback) {
    if (msg.header.category == CONTROL || msg.header.category == COMMAND || msg.header.category == ACKNOWLEDGEMENT) {
      CORE_LOG_ERROR("Attempting to send message {} which requires acknowledgment without acknowledgment handling", msg.header);
      return;
    }

    pending_response response{
      .header = msg.header,
      .sent_time = std::chrono::steady_clock::now(),
      .response_callback = response_callback,
      .timeout_callback = nullptr,
      .timer = asio::steady_timer(net_context->io_context),
    };
    CORE_LOG_DEBUG("PENDING-RESPONSE {}", response.header);

    {
      auto itr = std::find_if(pending_responses.begin(), pending_responses.end(), [&response](const pending_response& existing_response) {
        return existing_response.header == response.header;
      });
      OTHER_ASSERT(itr == pending_responses.end(), "Response for message ID {} already pending", response.header.id);
    }

    net_thread_message_bus.send_message(std::move(msg));
    auto resp_itr = pending_responses.insert(pending_responses.end(), std::move(response));
    OTHER_ASSERT(resp_itr != pending_responses.end(), "Failed to insert pending response for message ID {}", response.header.id);
  }

  void server::send_message_and_wait_response(message&& msg, microseconds timeout, pending_response::on_response response_callback, pending_response::on_timeout timeout_callback) {
    pending_response response{
      .header = msg.header,
      .sent_time = std::chrono::steady_clock::now(),
      .response_callback = response_callback,
      .timeout_callback = timeout_callback,
      .timer = asio::steady_timer(net_context->io_context),
    };
    CORE_LOG_DEBUG("PENDING-RESPONSE {} (timeout: {} us)", response.header, timeout.count());

    {
      auto itr = std::find_if(pending_responses.begin(), pending_responses.end(), [&response](const pending_response& existing_response) {
        return existing_response.header == response.header;
      });
      OTHER_ASSERT(itr == pending_responses.end(), "Response for message ID {} already pending", response.header.id);
    }

    net_thread_message_bus.send_message(std::move(msg));
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
  }

  natural_t server::set_timeout(microseconds duration, timeout::on_timeout timeout_callback) {
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

  void server::clear_timeout(natural_t timeout_id) {
    auto itr = std::ranges::find_if(pending_timeouts, [&](const timeout& t) { return t.id == timeout_id; });
    if (itr != pending_timeouts.end()) {
      itr->timer.cancel();
      pending_timeouts.erase(itr);
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

  task test_coroutine() {
    for (uint32_t i = 0; i < 3; ++i) {
      std::println("Coroutine tick {}", i);
      co_await std::suspend_always{};
    }
    co_return;
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

    itr->executable = replace_all_substrings_with(itr->executable.string(), "${configuration}", "Debug");
    CORE_LOG_DEBUG("Launching Other application executable '{}' @ [{}]:", itr->executable.string(), working_dir.string());
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
    itr->args.append_range(std::vector<std::string>{ "--port", std::to_string(main_binding_point.port) });
    launch_detached_process(itr->working_directory, itr->executable, itr->args);

    std::string ping_session_ev_name = "ping-session:[" + std::to_string(itr->id) + "]";
    events->register_timed_event(ping_session_ev_name, seconds(10), true);
    events->add_listener(ping_session_ev_name, [this, session_id = itr->id](const value& ec) {
      CORE_LOG_DEBUG("Pinging session [{}] to check connectivity", session_id);
      message msg;
      msg.header = {
        .category = CONTROL,
        .id = PING,
      };
      const uint8_t* id_bytes = reinterpret_cast<const uint8_t*>(&session_id);
      msg.data.append_range(std::span(id_bytes, sizeof(integer_t)));

      send_message_and_wait_acknowledgment(
        std::move(msg), seconds(1),
        std::bind_front(&server::on_ack_control_ping_network_thread, this),
        std::bind_front(&server::on_timeout_control_ping_network_thread, this)
      );
    });

#endif
  }

  void server::process_network_thread_messages(message&& msg) {
    CORE_LOG_TRACE("Processing message from network thread {} [{} bytes]", msg.header, msg.data.size());

    switch (msg.header.category) {
      case NOTIFICATION:
        switch (msg.header.id) {
          case SESSION_CLOSED: handle_notification_session_closed(std::move(msg)); break;
          default:
            CORE_LOG_ERROR("Server received unknown notification message ID {}", msg.header.id);
            break;
        }
        break;

      case ACKNOWLEDGEMENT:
        switch (msg.header.id) {
          case ACK: handle_acknowledgement_ack(std::move(msg)); break;
          default:
            CORE_LOG_ERROR("Server received unknown acknowledgment message ID {}", msg.header.id);
            break;
        }
        break;

      case CONTROL:
        switch (msg.header.id) {
          // case PING: handle_control_ping(std::move(msg)); break;
          case PONG: handle_control_pong(std::move(msg)); break;
          default:
            CORE_LOG_ERROR("Server received unknown CONTROL message ID {}", msg.header.id);
            break;
        }
        break;

      case RESPONSE: handle_response(std::move(msg)); break;

      default:
        CORE_LOG_ERROR("Server received unknown message category {}", msg.header.category);
        break;
    }
  }

  void server::on_ack_control_ping_network_thread(message_header header, const std::vector<uint8_t>& data) {
    // all good
  }

  void server::on_timeout_control_ping_network_thread(message_header header) {
    CORE_LOG_WARN("Network thread timed out waiting for PONG response");
    /// handle_network_thread_unresponsive();
  }

  void server::on_ack_session_listen_for_network_thread(message_header header, const std::vector<uint8_t>& data) {
    CORE_LOG_INFO("Network thread acknowledged event request at session check in for session [{}]", header.id);
    state_machine.handle_event(server_event::SERVER_EVENT_READY, this);

    /// register event for thread check in
    natural_t event_id = events->register_timed_event("status-check:[network-thread]", seconds(1), /* recurring = */ true);
    if (event_id == 0) {
      CORE_LOG_ERROR("Failed to register event for network thread check-in");
      return;
    }
    events->add_listener(event_id, [this](const value& ec) {
      message msg;
      msg.header = {
        .category = CONTROL,
        .id = PING,
      };
      netw_thread_heartbeat_timeout_id = set_timeout(milliseconds(250), [this](natural_t timeout_id) {
        CORE_LOG_ERROR("Network thread failed to respond to PING within timeout period");
        /// handle_network_thread_unresponsive();
      });

      net_thread_message_bus.send_message(std::move(msg));
    });
  }

  void server::on_timeout_session_listen_for_network_thread(message_header header) {
    CORE_LOG_WARN("Network thread timed out waiting for event request at session check in for session [{}]", header.id);
  }

  void server::on_ack_shutdown_request_network_thread(message_header header, const std::vector<uint8_t>& data) {
    CORE_LOG_DEBUG("Network thread acknowledged shutdown request");

    net_thread->shutdown();
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
        net_thread->force_shutdown();
        state_machine.handle_event(server_event::SERVER_EVENT_SHUT_DOWN, this);
        return;
      } catch (const std::exception& e) {
        CORE_LOG_ERROR("Error occurred while shutting down network thread: {}", e.what());
      }
    }

    CORE_LOG_TRACE("Retrying network thread shutdown...");
    on_shutdown_request();
  }

  void server::on_respond_session_check_in_network_thread(message_header header, const std::vector<uint8_t>& data) {
    OTHER_ASSERT(data.size() >= sizeof(integer_t), "Invalid session check in packet!");

    integer_t session_id = *reinterpret_cast<const integer_t*>(data.data());

    auto itr = std::ranges::find_if(pending_apps, [&](const other_application& app) { return session_id == app.id; });
    if (itr == pending_apps.end()) {
      CORE_LOG_ERROR("Server received a session check in for an unknown Other application : {}", session_id);
      return;
    }

    CORE_LOG_DEBUG("finalizing connection to pending application : {}", session_id);
    auto [app_itr, success] = other_apps.insert({ session_id, std::move(*itr) });
    if (!success || app_itr == other_apps.end()) {
      CORE_LOG_ERROR("Failed to save Other application session ID from check-in : {}", session_id);
      return;
    }
    pending_apps.erase(itr);

    CORE_LOG_DEBUG("Session {} connection finalized", session_id);
    app_itr->second.connected = true;
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

    events->clear();
    if (!net_context->io_context.stopped()) {
      net_context->io_context.stop();
    }
  }

  void server::handle_notification_session_closed(message&& msg) {
    OTHER_ASSERT(msg.data.size() >= sizeof(integer_t), "Invalid session closed notification size");
    integer_t session_id = *reinterpret_cast<const integer_t*>(msg.data.data());
    CORE_LOG_INFO("Session {} has been closed by the network thread", session_id);

    auto app_itr = other_apps.find(session_id);
    if (app_itr != other_apps.end()) {
      CORE_LOG_INFO("Other application [{}] has disconnected", app_itr->second.executable.string());

      /// cancel event
      std::string ping_session_ev_name = "ping-session:[" + std::to_string(session_id) + "]";
      events->cancel_event(ping_session_ev_name);

      app_itr->second.connected = false;
    } else {
      CORE_LOG_WARN("Received session closed notification for unknown other application with session ID {}", session_id);
    }
  }

  void server::handle_acknowledgement_ack(message&& msg) {
    CORE_LOG_DEBUG("  - ACK");
    if (msg.data.size() >= sizeof(message_header)) {
      message_header acked_header = *reinterpret_cast<const message_header*>(msg.data.data());
      auto itr = std::ranges::find_if(pending_acks, [&acked_header](const pending_ack& ack) { return ack.header == acked_header; });

      if (itr != pending_acks.end()) {
        CORE_LOG_DEBUG("Acknowledgment received for message {}", acked_header);
        itr->timer.cancel();
        if (itr->ack_callback) {
          CORE_LOG_TRACE("Invoking acknowledgment callback for message {}", acked_header);
          itr->ack_callback(acked_header, msg.data);
        }
        pending_acks.erase(itr);
      } else {
        CORE_LOG_ERROR("Received acknowledgment for unknown message {}", acked_header);
      }
    } else {
      CORE_LOG_WARN("Received invalid acknowledgment message size");
    }
  }

  void server::handle_control_pong(message&& msg) {
    size_t cursor = 0;
    integer_t session = serialization::read_value<integer_t>(msg.data, cursor);

    if (session == 0) {
      clear_timeout(netw_thread_heartbeat_timeout_id);
    }
    /// else handle real session
    else {
      /// \todo
    }
  }

  void server::handle_response(message&& msg) {
    CORE_LOG_DEBUG("  - RESPONSE");
    auto itr = std::ranges::find_if(pending_responses, [&msg](const pending_response& response) { return response.header.id == msg.header.id; });

    if (itr != pending_responses.end()) {
      CORE_LOG_DEBUG("Response received for message {}", msg.header);
      if (itr->response_callback) {
        CORE_LOG_TRACE("Invoking response callback for message {}", msg.header);
        itr->response_callback(msg.header, msg.data);
      }
      pending_responses.erase(itr);
    } else {
      CORE_LOG_ERROR("Received response for unknown message {}", msg.header);
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
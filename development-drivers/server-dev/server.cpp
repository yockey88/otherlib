/**
 * \file server-dev/server.cpp
 **/
#include "server.hpp"

#include <filesystem>

#include "core/defines.hpp"
#include "thread/message.hpp"

#include "renderer/ui/ui_helpers.hpp"

#include "rendering-pipelines/empty_pipeline.hpp"

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
    events = make_scope<event_system>(net_context->io_context);

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

      ui_ptr = make_scope<server_ui>(renderer, project_cache);
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
    events->poll();
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

  void server::send_message_no_acknowledgment(message&& msg, pending_response::on_response response_callback) {
    if (msg.header.category == CONTROL || msg.header.category == COMMAND || msg.header.category == ACKNOWLEDGEMENT) {
      CORE_LOG_ERROR("Attempting to send message {} which requires acknowledgment without acknowledgment handling", msg.header);
      return;
    }

    pending_response response{
      .header = msg.header,
      .sent_time = std::chrono::steady_clock::now(),
      .response_callback = response_callback,
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

  void server::send_message_and_wait_acknowledgment(message&& msg, std::chrono::microseconds timeout, pending_ack::on_ack ack_callback, pending_ack::on_timeout timeout_callback) {
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
      OTHER_ASSERT(itr == pending_acks.end(), "Acknowledgment for message ID {} already pending", ack.header.id);
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
    });
  }

  void server::begin_other_application(const filepath& working_dir, const filepath& exe_name, const std::vector<std::string>& args) {
    /// launch other application with command line args specifying the server's current command port, the project configuration
    ///   and the working directory
    /// set an event listener for 'session-initial-check-in' to register other-application with the server
    /// respond to the ping with server data

    static integer_t next_id = 1;
    integer_t id = next_id++;
    auto itr = pending_apps.insert(pending_apps.end(), other_application{ .id = id, .working_directory = working_dir, .executable = exe_name, .args = args });
    OTHER_ASSERT(itr != pending_apps.end(), "Failed to begin Other application : {}  [{}]", id, exe_name.string());
    CORE_LOG_DEBUG("Starting Other application : {}", id);

    {
      message msg;
      msg.header = {
        .category = REQUEST,
        .id = SESSION_CHECK_IN,
      };

      const uint8_t* id_bytes = reinterpret_cast<const uint8_t*>(&id);
      msg.data.append_range(std::span(id_bytes, sizeof(integer_t)));
      send_message_no_acknowledgment(std::move(msg), std::bind_front(&server::on_respond_session_check_in_network_thread, this));
    }

    itr->args.append_range(std::vector<std::string>{ "--sid", std::to_string(itr->id) });
    itr->args.append_range(std::vector<std::string>{ "--port", std::to_string(main_binding_point.port) });
    launch_detached_process(itr->working_directory, itr->executable, itr->args);

    std::string ping_session_ev_name = "ping-session:[" + std::to_string(itr->id) + "]";
    // natural_t ping_event_id = register_event(
  }

  void server::core_update() {
    net_context->io_context.poll();
    if (net_context->io_context.stopped()) {
      net_context->io_context.restart();
    }
    events->poll();

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

  void server::on_ack_session_listen_for_network_thread(message_header header, const std::vector<uint8_t>& data) {
    CORE_LOG_INFO("Network thread acknowledged event request at session check in for session [{}]", header.id);
    state_machine.handle_event(server_event::SERVER_EVENT_READY, this);

    /// register event for thread check in
    natural_t event_id = events->register_event("status-check:[network-thread]", std::chrono::seconds(1), /* recurring = */ true);
    if (event_id == 0) {
      CORE_LOG_ERROR("Failed to register event for network thread check-in");
      return;
    }
    events->add_listener(event_id, [this](const value& ec) {
      CORE_LOG_DEBUG("Network thread check-in event fired, checking for network thread status...");
    });

    // begin_other_application(std::filesystem::current_path(), "build/development-drivers/Debug/runtime_dev.exe", { "resources/dev-config.toml" });
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

    events->cancel_all();

    if (!net_context->io_context.stopped()) {
      net_context->io_context.stop();
    }

    // for (auto itr = other_apps.begin(); itr != other_apps.end();) {
    //   CORE_LOG_DEBUG(" - clearing other application session {}", itr->first);
    //   itr = other_apps.erase(itr);
    // }
    // for (auto itr = pending_apps.begin(); itr != pending_apps.end();) {
    //   CORE_LOG_DEBUG(" - clearing pending other application session {}", itr->id);
    //   itr = pending_apps.erase(itr);
    // }

    // for (auto itr = pending_events.begin(); itr != pending_events.end();) {
    //   itr->timer.cancel();
    //   itr = pending_events.erase(itr);
    // }
    // for (auto itr = pending_acks.begin(); itr != pending_acks.end();) {
    //   itr->timer.cancel();
    //   itr = pending_acks.erase(itr);
    // }
    // pending_responses.clear();
  }

  void server::handle_notification_session_closed(message&& msg) {
    OTHER_ASSERT(msg.data.size() >= sizeof(integer_t), "Invalid session closed notification size");
    integer_t session_id = *reinterpret_cast<const integer_t*>(msg.data.data());
    CORE_LOG_INFO("Session {} has been closed by the network thread", session_id);

    auto app_itr = other_apps.find(session_id);
    if (app_itr != other_apps.end()) {
      CORE_LOG_INFO("Other application [{}] has disconnected", app_itr->second.executable.string());
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
    // CORE_LOG_DEBUG("Received PONG from network thread");

    // switch (state_machine.get_current_state()) {
    //   case server_state::SERVER_STATE_INITIALIZING:
    //     state_machine.handle_event(server_event::SERVER_EVENT_THREAD_CHECK_IN, this);
    //     break;
    //   default:
    //     CORE_LOG_WARN("Received PONG in unexpected state {}", static_cast<int>(state_machine.get_current_state()));
    //     break;
    // }
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

}  // namespace other
/**
 * \file driver/systems/network_system.cpp
 **/
#include "driver/systems/network_system.hpp"

#include "core/defines.hpp"

#include "driver/driver.hpp"
#include "driver/systems/scene_system.hpp"

namespace other {

  void network_system::initialize(driver_kernel* kernel) {
    net_context = make_scope<network_context>();
    OTHER_ASSERT(net_context != nullptr, "Failed to create network context.");
    struct signal_catcher {
      signal_catcher(network_system* network_system_ptr)
          : network_system_ptr(network_system_ptr) {}
      void catch_signal(std::error_code ec, int signum) {
        if ((ec && ec == asio::error::operation_aborted) ||
            network_system_ptr == nullptr || network_system_ptr->net_context == nullptr) {
          return;
        }

        if (!ec) {
          network_system_ptr->catch_signal(signum);
        } else {
          CORE_LOG_ERROR("Error while waiting for signal: {}", ec.message());

          if (network_system_ptr->get_driver().current_driver_state() == driver_state::DRIVER_STATE_RUNNING) {
            network_system_ptr->net_context->signals.async_wait(std::bind_front(&signal_catcher::catch_signal, this));
          }
        }
      }

      network_system* network_system_ptr = nullptr;
    };

    static signal_catcher catcher{ this };
    net_context->signals.async_wait(std::bind_front(&signal_catcher::catch_signal, &catcher));
    net_context->net_thread = make_scope<network_thread>(net_context->net_thread_message_bus);
    net_context->net_thread->launch();
    net_context->net_thread_message_bus.register_thread();

    bool force_disable_network = get_driver().get_config_value<bool>("networking.force-disable", false);
    if (!force_disable_network) {
      start_network(kernel);
    }
  }

  void network_system::tick(driver_kernel* kernel, double dt) {
    OTHER_ASSERT(net_context != nullptr, "Network context is not initialized in network system.");
    if (!net_context->io_context.stopped()) {
      net_context->io_context.poll();
    }

    auto msg_opt = net_context->net_thread_message_bus.receive_message();
    if (msg_opt.has_value()) {
      process_network_thread_messages(kernel, std::move(*msg_opt));
    }
  }

  void network_system::shutdown(driver_kernel* kernel) {
    OTHER_ASSERT(net_context != nullptr, "Network context is not initialized in network system.");
    net_context->net_thread->wait_for_shutdown_complete();
    net_context->net_thread = nullptr;
    net_context = nullptr;
  }

  void network_system::start_network(driver_kernel* kernel) {
    // now we kick off main networking session
    bool force_disable_network = get_driver().get_config_value<bool>("networking.force-disable", false);
    bool network_thread_active = !force_disable_network;

    std::string role = get_driver().get_config_value<std::string>("application.role", "client");
    if (role != "client" && role != "server") {
      CORE_LOG_WARN("Unknown application role '{}', defaulting to 'client'", role);
      role = "client";
    }

    if (role == "client") {
      primary_role = role::CLIENT;
    } else if (role == "server") {
      primary_role = role::SERVER;
    }

    if (force_disable_network) {
      primary_role = role::NONE;
    }

    CORE_LOG_INFO("Network Role : [{}]", primary_role);
    if (network_thread_active && primary_role == role::CLIENT) {
      message connect_msg;
      connect_msg.header = {
        .category = COMMAND,
        .id = SESSION_CONNECT_TO,
      };

      command_session_connect_to conn_cmd;
      conn_cmd.address = net_context->main_binding_point;
      connect_msg.data.append_range(conn_cmd.as_buffer());
      send_message_and_wait_acknowledgment(
        kernel, std::move(connect_msg), std::chrono::seconds(10),
        message_handler{
          [this, kernel](message_header h, std::span<const uint8_t> d) { on_ack_session_connect_to(kernel, h, d); },
          [this, kernel](message_header h) { on_timeout_session_connect_to(kernel, h); },
        }
      );
    } else if (network_thread_active && primary_role == role::SERVER) {
      message msg;
      msg.header = {
        .category = COMMAND,
        .id = SESSION_LISTEN_FOR,
      };

      command_session_listen_at listen_cmd;
      listen_cmd.address = net_context->main_binding_point;
      msg.data.append_range(listen_cmd.as_buffer());
      send_message_and_wait_acknowledgment(
        kernel, std::move(msg), std::chrono::seconds(10),
        message_handler{
          [this, kernel](message_header h, std::span<const uint8_t> d) { on_ack_session_listen_for_network_thread(kernel, h, d); },
          [this, kernel](message_header h) { on_timeout_session_listen_for_network_thread(kernel, h); },
        }
      );
    } else {
      CORE_LOG_WARN("Network thread is disabled, running in offline mode.");
    }
  }

  void network_system::begin_shutdown_sequence(driver_kernel* kernel) {
    net_context->signals.cancel();

    for (auto& ack : ack_list.pending_acks) {
      ack.timer.cancel();
    }
    for (auto& response : resp_list.pending_responses) {
      response.timer.cancel();
    }

    CORE_LOG_DEBUG("Sending shutdown request to network thread...");
    message msg;
    msg.header = {
      .category = COMMAND,
      .id = SHUTDOWN_REQUEST,
    };

    send_message_and_wait_acknowledgment(
      kernel, std::move(msg), std::chrono::milliseconds(250),
      message_handler{
        [this, kernel](message_header h, std::span<const uint8_t> d) { on_ack_shutdown_request_network_thread(kernel, h, d); },
        [this, kernel](message_header h) { on_timeout_shutdown_request_network_thread(kernel, h); },
      }
    );
  }

  void network_system::send_to_network_thread(driver_kernel* kernel, message&& msg) {
    OTHER_ASSERT(net_context != nullptr, "Network context is not initialized in network system.");
    net_context->net_thread_message_bus.send_message(std::move(msg));
  }

  void network_system::send_message_and_detach_acknowledgement(driver_kernel* kernel, message&& msg, message_handler handler) {
    acknowledgement_list::pending_ack ack{
      .header = msg.header,
      .handler = handler,
      .timer = asio::steady_timer(net_context->io_context),
    };

    {
      auto itr = std::find_if(ack_list.pending_acks.begin(), ack_list.pending_acks.end(), [&ack](const acknowledgement_list::pending_ack& existing_ack) {
        return existing_ack.header == ack.header;
      });
      OTHER_ASSERT(itr == ack_list.pending_acks.end(), "Acknowledgment for message ID {} already pending", ack.header);
    }

    send_to_network_thread(kernel, std::move(msg));
    auto ack_itr = ack_list.pending_acks.insert(ack_list.pending_acks.end(), std::move(ack));
    OTHER_ASSERT(ack_itr != ack_list.pending_acks.end(), "Failed to insert pending acknowledgment for message ID {}", ack.header.id);
  }

  natural_t network_system::send_message_and_wait_acknowledgment(driver_kernel* kernel, message&& msg, microseconds timeout, message_handler handler) {
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
    send_to_network_thread(kernel, std::move(msg));
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
          itr->handler.on_timeout(itr->header);
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

  void network_system::cancel_acknowledgment(natural_t ack_id) {
    auto itr = std::ranges::find_if(ack_list.pending_acks, [&](const acknowledgement_list::pending_ack& ack) { return ack.id == ack_id; });
    if (itr != ack_list.pending_acks.end()) {
      CORE_LOG_DEBUG("Cancelling pending acknowledgment for message ID {}", itr->header.id);
      itr->timer.cancel();
      ack_list.pending_acks.erase(itr);
    }
  }

  void network_system::send_message_and_detach_response(driver_kernel* kernel, message&& msg, message_handler handler) {
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

    send_to_network_thread(kernel, std::move(msg));
    if (handler.handle_msg == nullptr) {
      return;
    }

    auto resp_itr = resp_list.pending_responses.insert(resp_list.pending_responses.end(), std::move(response));
    OTHER_ASSERT(resp_itr != resp_list.pending_responses.end(), "Failed to insert pending response for message ID {}", response.header.id);
  }

  natural_t network_system::send_message_and_wait_response(driver_kernel* kernel, message&& msg, microseconds timeout, message_handler handler) {
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

    send_to_network_thread(kernel, std::move(msg));

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
      itr->handler.on_timeout(itr->header);

      resp_list.pending_responses.erase(itr);
    });

    return resp_itr->id;
  }

  void network_system::cancel_response(natural_t response_id) {
    auto itr = std::ranges::find_if(resp_list.pending_responses, [&](const response_list::pending_response& resp) { return resp.id == response_id; });
    if (itr != resp_list.pending_responses.end()) {
      CORE_LOG_DEBUG("Cancelling pending response for message ID {}", itr->header.id);
      itr->timer.cancel();
      resp_list.pending_responses.erase(itr);
    }
  }

  void network_system::catch_signal(int signum) {
    if (signum == SIGINT || signum == SIGTERM) {
      CORE_LOG_INFO("Received signal {}, shutting down driver...", signum);
      get_driver().request_shutdown();
    } else {
      CORE_LOG_WARN("Received unhandled signal {}", signum);
    }
  }

  natural_t network_system::set_timeout(microseconds duration, timer_list::timeout::on_timeout timeout_callback) {
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

  void network_system::clear_timeout(natural_t timeout_id) {
    auto itr = std::ranges::find_if(timeout_list.pending_timeouts, [&](const timer_list::timeout& t) { return t.id == timeout_id; });
    if (itr != timeout_list.pending_timeouts.end()) {
      itr->timer.cancel();
      timeout_list.pending_timeouts.erase(itr);
    }
  }

  asio::io_context& network_system::io_context() {
    OTHER_ASSERT(net_context != nullptr, "Network context is not initialized in network system.");
    return net_context->io_context;
  }

  message_bus& network_system::net_message_bus() {
    OTHER_ASSERT(net_context != nullptr, "Network context is not initialized in network system.");
    return net_context->net_thread_message_bus;
  }

  bool network_system::network_active() const {
    return primary_role != NONE;
  }

  void network_system::register_other_application(driver_kernel* kernel, integer_t session_id, application_list::other_application* app) {
    OTHER_ASSERT(app != nullptr, "Application pointer is null after insertion!");

    app->connected = true;
    CORE_LOG_INFO("Other application [{}] has connected", session_id);
    session_application_information_request(kernel, session_id);
  }

  void network_system::request_scene_udp_binding(driver_kernel* kernel, udp_binding_information address) {
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

    send_message_and_wait_response(
      kernel, std::move(udp_request), seconds(5),
      message_handler{
        [this, kernel](message_header h, std::span<const uint8_t> data) { on_respond_new_udp_stream_binding(kernel, h, data); },
        [this, kernel](message_header h) { on_timeout_new_udp_stream_binding(kernel, h); },
      }
    );
  }

  void network_system::process_network_thread_messages(driver_kernel* kernel, message&& msg) {
    switch (msg.header.category) {
      case NOTIFICATION:
        switch (msg.header.id) {
          case STREAM_RX_UDP_DATAGRAM: handle_notification_stream_receive_udp_datagram(kernel, std::move(msg)); break;
          case SESSION_CHECK_IN: handle_notification_session_check_in(kernel, std::move(msg)); break;
          case SESSION_CLOSED: handle_notification_session_closed(kernel, std::move(msg)); break;
          case NETWORK_THREAD_READY: handle_notification_network_thread_ready(kernel, std::move(msg)); break;
          case NETWORK_THREAD_SHUTDOWN_COMPLETE: handle_notification_network_thread_shutdown_complete(kernel, std::move(msg)); break;
          default:
            CORE_LOG_ERROR("Server received unknown notification message ID {}", msg.header.id);
            break;
        }
        break;

      case ACKNOWLEDGEMENT:
        switch (msg.header.id) {
          case ACK: handle_acknowledgement_ack(kernel, std::move(msg)); break;
          default: CORE_LOG_ERROR("Driver received unknown acknowledgment message ID {}", msg.header.id); break;
        }
        break;

      case CONTROL:
        switch (msg.header.id) {
          case PING: handle_control_ping(kernel, std::move(msg)); break;
          case PONG: handle_control_pong(kernel, std::move(msg)); break;
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

      case RESPONSE: handle_response(kernel, std::move(msg)); break;

      case SESSION_EVENT:
        switch (msg.header.id) {
          case SESSION_RX_MESSAGE: handle_session_event_rx_message(kernel, std::move(msg)); break;
          default:
            CORE_LOG_ERROR("Server received unknown SESSION_EVENT message ID {}", msg.header.id);
            break;
        }
        break;

      case ERROR_ALERT: get_driver().handle_error_alert(std::move(msg)); break;

      default:
        CORE_LOG_ERROR("Server received unknown message category {}", msg.header.category);
        break;
    }
  }

  void network_system::on_ack_command_environment_load_scene(driver_kernel* kernel, message_header header, std::span<const uint8_t> data) {
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

    CORE_LOG_INFO("ENVIRONMENT_LOAD_SCENE command acknowledged by [session {}].", ackmsg.session_id <= 0 ? "<self>" : std::to_string(ackmsg.session_id));
    CORE_LOG_INFO("  - Requires UDP Binding: {}", binding_info.endpoint.port != 0 && binding_info.remote_endpoint.port != 0 ? "Yes" : "No");
    if (binding_info.endpoint.port != 0 || binding_info.remote_endpoint.port != 0) {
      CORE_LOG_INFO("   - Check-in Hash: {}", binding_info.check_in_hash);
      CORE_LOG_INFO("   - Local Endpoint: [{}]", binding_point::write_string(binding_info.endpoint));
      CORE_LOG_INFO("   - Remote Endpoint: [{}]", binding_point::write_string(binding_info.remote_endpoint));
      request_scene_udp_binding(kernel, binding_info);
    }
  }

  void network_system::on_timeout_environment_load_scene(driver_kernel* kernel, message_header header) {
    CORE_LOG_ERROR("Timeout while waiting for acknowledgment of ENVIRONMENT_LOAD_SCENE command (header: {})", header);
    CORE_LOG_WARN("Does the active project have a scene using that name already?");

    auto& scenes = kernel->get_core_system<scene_system>();
    scenes.unload_active_scene();
  }

  void network_system::on_ack_shutdown_request_network_thread(driver_kernel* kernel, message_header header, const std::span<const uint8_t> data) {
    OTHER_ASSERT(net_context != nullptr, "Network context is null in driver.");
    OTHER_ASSERT(net_context->net_thread != nullptr, "Network thread is null in driver.");
    CORE_LOG_DEBUG("Network thread acknowledged shutdown request");

    net_context->net_thread->shutdown();
  }

  void network_system::on_timeout_shutdown_request_network_thread(driver_kernel* kernel, message_header header) {
    /// force shutdown
    OTHER_ASSERT(net_context != nullptr, "Network context is null in driver.");
    OTHER_ASSERT(net_context->net_thread != nullptr, "Network thread is null in driver.");
    CORE_LOG_ERROR("Timeout waiting for network thread to acknowledge shutdown request");
    CORE_LOG_ERROR("   Data may be corrupt from unclean shutdown");

    net_context->net_thread->force_shutdown();
  }

  void network_system::on_ack_session_connect_to(driver_kernel* kernel, message_header header, const std::span<const uint8_t> data) {
    session_connect_to_response response = other_message_spec::parse<session_connect_to_response>(data);
    if (response.ack_nack == 0x01) {
      CORE_LOG_INFO("Connected to session [{}] successfully.", response.session_id);
    } else {
      CORE_LOG_ERROR("Failed to connect to session at [{}]", response.session_id);
      return;
    }

    auto& scenes = kernel->get_core_system<scene_system>();
    scene* active_scene = scenes.get_active_scene();
    if (active_scene != nullptr) {
      /// if we connected (ack == 1) then we have to synchronize with remote
      active_scene->synchronized = (response.ack_nack == 0x00);
    }

    /// server does this in @ref network_system::on_ack_session_listen_for_network_thread
    if (primary_role == role::CLIENT) {
      get_driver().process_driver_event(driver_event::DRIVER_EVENT_READY);
    }
  }

  void network_system::on_timeout_session_connect_to(driver_kernel* kernel, message_header header) {
    CORE_LOG_ERROR("Timeout while waiting for SESSION_CONNECT_TO response (header: {})", header);

    auto& scenes = kernel->get_core_system<scene_system>();
    scene* active_scene = scenes.get_active_scene();
    if (active_scene == nullptr) {
      /// there is no session so we cannot be synchronized
      active_scene->synchronized = true;
    }

    /// server does this in @ref network_system::on_timeout_session_listen_for_network_thread
    if (primary_role == role::CLIENT) {
      get_driver().process_driver_event(driver_event::DRIVER_EVENT_READY);
    }
  }

  void network_system::on_ack_session_listen_for_network_thread(driver_kernel* kernel, message_header header, const std::span<const uint8_t> data) {
    CORE_LOG_INFO("Network thread acknowledged event request at session check in for session [{}]", header.id);

    auto& events = get_driver().get_event_system();
    OTHER_ASSERT(events != nullptr, "Event system is null in driver.");

    /// register event for thread check in
    natural_t event_id = events->register_timed_event("status-check:[network-thread]", seconds(1), /* recurring = */ true);
    if (event_id == 0) {
      CORE_LOG_ERROR("Failed to register event for network thread check-in");
      return;
    }

    events->add_listener(event_id, [this, kernel](const value& ec) {
      message msg;
      msg.header = {
        .category = CONTROL,
        .id = PING,
      };

      control_ping ping_msg;

      net_context->netw_thread_heartbeat_timeout_id = set_timeout(milliseconds(250), [this, kernel](natural_t timeout_id) {
        CORE_LOG_ERROR("Network thread failed to respond to PING within timeout period");
        // get_driver().handle_network_thread_unresponsive();
      });

      send_to_network_thread(kernel, std::move(msg));
    });

    /// client does this in @ref network_system::on_ack_session_connect_to
    if (primary_role == role::SERVER) {
      get_driver().process_driver_event(driver_event::DRIVER_EVENT_READY);
    }
  }

  void network_system::on_timeout_session_listen_for_network_thread(driver_kernel* kernel, message_header header) {
    CORE_LOG_WARN("Network thread timed out waiting for event request at session check in for session [{}]", header.id);

    /// client does this in @ref network_system::on_timeout_session_connect_to
    if (primary_role == role::SERVER) {
      get_driver().process_driver_event(driver_event::DRIVER_EVENT_READY);
    }
  }

  void network_system::on_respond_session_check_in(driver_kernel* kernel, message_header header, const std::span<const uint8_t> data) {
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
    register_other_application(kernel, session_id, &app_itr->second);
  }

  void network_system::on_respond_new_udp_stream_binding(driver_kernel* kernel, message_header header, std::span<const uint8_t> data) {
    auto& scenes = kernel->get_core_system<scene_system>();
    scene* active_scene = scenes.get_active_scene();
    if (active_scene == nullptr) {
      CORE_LOG_ERROR("No active scene to set UDP handle on for NEW_UDP_STREAM_BINDING response.");
      return;
    }

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

    if (primary_role == role::CLIENT) {
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
      send_message_and_detach_response(kernel, std::move(msg), {});

    } else {
      CORE_LOG_DEBUG("UDP binding ID: {} waiting for check-in from server.", binding_id);
    }
  }

  void network_system::on_timeout_new_udp_stream_binding(driver_kernel* kernel, message_header header) {
    CORE_LOG_ERROR("Timeout while waiting for NEW_UDP_STREAM_BINDING response (header: {})", header);
  }

  void network_system::handle_notification_stream_receive_udp_datagram(driver_kernel* kernel, message&& msg) {
    notification_stream_rx_datagram udp_msg = other_message_spec::parse<notification_stream_rx_datagram>(msg.data);
    integer_t stream_id = udp_msg.stream_id;
    udp_datagram datagram = udp_msg.datagram;

    CORE_LOG_DEBUG("Received UDP datagram on stream ID: {} of type: {}", stream_id, static_cast<uint8_t>(datagram.type));
    auto& scenes = kernel->get_core_system<scene_system>();
    scene* active_scene = scenes.get_active_scene();
    if (active_scene == nullptr) {
      CORE_LOG_ERROR("No active scene to handle incoming UDP datagram.");
      return;
    }

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

  void network_system::handle_notification_session_check_in(driver_kernel* kernel, message&& msg) {
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
    if (primary_role == role::SERVER) {
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
      register_other_application(kernel, session_id, app);
    } else if (primary_role == role::CLIENT) {
      client_session_id = session_id;
    }
  }

  void network_system::handle_notification_session_closed(driver_kernel* kernel, message&& msg) {
    notification_session_closed closed = other_message_spec::parse<notification_session_closed>(msg.data);
    integer_t session_id = closed.session_id;
    if (session_id < 0) {
      CORE_LOG_ERROR("Invalid session ID received in session closed notification: {}", session_id);
      return;
    }

    CORE_LOG_INFO("Session [{}] closed.", session_id);
    get_driver().on_notification_session_closed(session_id);
  }

  void network_system::handle_notification_network_thread_ready(driver_kernel* kernel, message&& msg) {
    get_driver().confirm_initialization();
  }

  void network_system::handle_notification_network_thread_shutdown_complete(driver_kernel* kernel, message&& msg) {
    net_context->net_thread->wait_for_shutdown_complete();
    CORE_LOG_DEBUG("Network thread has fully stopped.");

    net_context->io_context.stop();

    ack_list.pending_acks.clear();
    resp_list.pending_responses.clear();
    timeout_list.pending_timeouts.clear();
    get_driver().confirm_shutdown();
  }

  void network_system::handle_acknowledgement_ack(driver_kernel* kernel, message&& msg) {
    acknowledgement ackmsg = other_message_spec::parse<acknowledgement>(msg.data);
    message_header acked_header = ackmsg.acked_header;

    auto itr = std::find_if(ack_list.pending_acks.begin(), ack_list.pending_acks.end(), [&](const acknowledgement_list::pending_ack& ack) {
      return acked_header == ack.header;
    });
    if (itr == ack_list.pending_acks.end()) {
      CORE_LOG_ERROR("Received acknowledgment for unknown message {}", acked_header);
      return;
    }

    itr->timer.cancel();
    if (ackmsg.ack_nack == 0x01) {
      CORE_LOG_DEBUG("  - ACK");
    } else {
      CORE_LOG_DEBUG("  - NACK");
    }

    if (itr->handler.handle_msg) {
      CORE_LOG_TRACE("Invoking acknowledgment callback for message {}", acked_header);
      itr->handler.handle_msg(acked_header, ackmsg.extra_data);
    }
  }

  void network_system::handle_control_ping(driver_kernel* kernel, message&& msg) {
    control_ping ping_msg = other_message_spec::parse<control_ping>(msg.data);
    if (ping_msg.session_id == 0) {
      /// respond to ping
      message pong_msg;
      pong_msg.header = {
        .category = CONTROL,
        .id = PONG,
      };

      control_pong pong;
      pong.session_id = 0;
      pong_msg.data.append_range(pong.as_buffer());

      send_to_network_thread(kernel, std::move(pong_msg));
    }
  }

  void network_system::handle_control_pong(driver_kernel* kernel, message&& msg) {
    control_pong pong_msg = other_message_spec::parse<control_pong>(msg.data);
    if (pong_msg.session_id == 0) {
      clear_timeout(net_context->netw_thread_heartbeat_timeout_id);
    }
    /// else handle real session
    else {
      /// \todo
    }
  }

  void network_system::handle_command_environment_load_scene(driver_kernel* kernel, integer_t session_id, message&& msg) {
    command_load_scene scene_cmd = other_message_spec::parse<command_load_scene>(msg.data);

    // auto& scenes = kernel->get_core_system<scene_system>();
    // natural_t scene_id = scenes.create_empty_scene(scene_cmd.scene_name);
    // scenes.set_scene_to_active(scene_id);

    // scene* active_scene = scenes.get_active_scene();
    // OTHER_ASSERT(active_scene != nullptr, "Failed to set active scene after loading empty scene");

    // /// if we received this them we are the 'server' part of the UDP stream (i.e. currently hosting the scene)
    // /// so we name these in terms of us being the server
    // /// \todo check if these are ok, and if not response with better ones
    // udp_binding_information binding_info;
    // binding_info.endpoint = scene_cmd.server_udp_address;
    // binding_info.remote_endpoint = scene_cmd.udp_address;
    // binding_info.check_in_hash = active_scene->id;

    // CORE_LOG_DEBUG("Suggested Scene Endpoints local = [{}], remote = [{}]", binding_point::write_string(binding_info.endpoint), binding_point::write_string(binding_info.remote_endpoint));
    // bool request_udp_binding = scene_cmd.requires_udp_binding == 0x01;
    // if (request_udp_binding) {
    //   request_scene_udp_binding(kernel, binding_info);
    // } else {
    //   CORE_LOG_DEBUG("Scene '{}' does not require UDP binding.", scene_cmd.scene_name);
    // }

    // /// first acknowledge that we loaded the scene
    // {
    //   message ack_msg;
    //   ack_msg.header = {
    //     .category = ACKNOWLEDGEMENT,
    //     .id = ACK,
    //   };

    //   acknowledgement ackmsg;
    //   ackmsg.session_id = session_id;
    //   ackmsg.acked_header = msg.header;
    //   ackmsg.ack_nack = 0x01;

    //   /// \todo send back final addressess, currently just echoing what was sent
    //   udp_binding_information client_binding_info;
    //   client_binding_info.endpoint = scene_cmd.udp_address;
    //   client_binding_info.remote_endpoint = scene_cmd.server_udp_address;
    //   client_binding_info.check_in_hash = active_scene->id;
    //   ackmsg.extra_data.append_range(client_binding_info.as_buffer());
    //   ack_msg.data.append_range(ackmsg.as_buffer());

    //   CORE_LOG_DEBUG("acknowledging ENVIRONMENT_LOAD_SCENE command");
    //   message tx_msg;
    //   tx_msg.header = {
    //     .category = COMMAND,
    //     .id = SESSION_TX_MESSAGE,
    //   };

    //   command_session_tx_message tx_session_msg;
    //   tx_session_msg.session_id = session_id;
    //   tx_session_msg.msg = std::move(ack_msg);
    //   tx_msg.data.append_range(tx_session_msg.as_buffer());

    //   send_to_network_thread(kernel, std::move(tx_msg));
    // }

    // /// scene is empty, so we are already up too date
    // if (scene_cmd.empty_scene_flag == 0x01) {
    //   CORE_LOG_DEBUG("Scene '{}' is empty, no scene data to request.", scene_cmd.scene_name);
    //   return;
    // }
    // active_scene->connect_remote_session(session_id);
  }

  void network_system::handle_request_session_information(driver_kernel* kernel, integer_t session_id, message&& msg) {
    session_information_response response;
    response.name = get_driver().get_project_name();
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

    send_message_and_detach_response(kernel, std::move(tx_msg), {});
  }

  void network_system::session_check_in_request(driver_kernel* kernel, integer_t session_id) {
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

  void network_system::session_application_information_request(driver_kernel* kernel, integer_t session_id, application_list::other_application* app) {
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
      print_session_information(kernel, app);
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

    send_to_network_thread(kernel, std::move(msg));
  }

  void network_system::handle_response(driver_kernel* kernel, message&& msg) {
    CORE_LOG_DEBUG("  - RESPONSE");
    auto itr = std::ranges::find_if(resp_list.pending_responses, [&msg](const response_list::pending_response& response) { return response.header.id == msg.header.id; });

    if (itr != resp_list.pending_responses.end()) {
      CORE_LOG_DEBUG("Response received for message {}", msg.header);
      if (itr->handler.handle_msg != nullptr) {
        CORE_LOG_TRACE("Invoking response callback for message {}", msg.header);
        itr->handler.handle_msg(msg.header, msg.data);
      }
      resp_list.pending_responses.erase(itr);
    } else {
      CORE_LOG_ERROR("Received response for unknown message {}", msg.header);
    }
  }

  void network_system::handle_response_session_information(driver_kernel* kernel, integer_t session_id, message&& msg) {
    session_information_response response = other_message_spec::parse<session_information_response>(std::span(msg.data));
    handle_session_information_response(kernel, session_id, std::move(response));
  }

  void network_system::print_session_information(driver_kernel* kernel, application_list::other_application* app) {
    CORE_LOG_INFO("Other application session [{}] information:", app->id);
    CORE_LOG_INFO("   - Name: {}", app->get_name());
    CORE_LOG_INFO("   - Executable: {}", app->executable.has_value() ? app->executable->string() : "<none>");
    CORE_LOG_INFO("   - Working Directory: {}", app->working_directory.has_value() ? app->working_directory->string() : "<none>");
  }

  void network_system::handle_session_event_rx_message(driver_kernel* kernel, message&& msg) {
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
          case ENVIRONMENT_LOAD_SCENE: handle_command_environment_load_scene(kernel, session_id, std::move(rx_msg)); return;
          default:
            CORE_LOG_WARN("Received unknown session event command message ID: {}", rx_msg.header.id);
            break;
        }
        break;

      case REQUEST:
        switch (rx_msg.header.id) {
          case SESSION_INFORMATION: handle_request_session_information(kernel, session_id, std::move(rx_msg)); return;
          default:
            CORE_LOG_WARN("Received unknown session event request message ID: {}", rx_msg.header.id);
            break;
        }
        break;

      case RESPONSE:
        switch (rx_msg.header.id) {
          case SESSION_INFORMATION: handle_response_session_information(kernel, session_id, std::move(rx_msg)); return;
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

  void network_system::handle_session_information_response(driver_kernel* kernel, integer_t session_id, session_information_response&& response) {
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

    print_session_information(kernel, &app);
  }

}  // namespace other
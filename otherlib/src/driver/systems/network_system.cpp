/**
 * \file driver/systems/network_system.cpp
 **/
#include "driver/systems/network_system.hpp"

#include <cstdint>

#include "core/defines.hpp"
#include "core/time.hpp"

#include "network/messages.hpp"

#include "driver/driver.hpp"

namespace other {

  void network_system::initialize(driver_kernel* kernel) {
    net_context = make_scope<network_context>();
    OTHER_ASSERT(net_context != nullptr, "Failed to create network context.");

    /// \todo move this somewhere more permanent?
    struct signal_catcher {
      signal_catcher(network_system* network_system_ptr)
          : network_system_ptr(network_system_ptr) {}
      void catch_signal(std::error_code ec, int signum) {
        if (ec && ec == asio::error::operation_aborted) {
          CORE_LOG_TRACE("Signal wait aborted");
          return;
        }

        if (!ec) {
          network_system_ptr->catch_signal(signum);
        } else {
          CORE_LOG_ERROR("Error while waiting for signal: {}", ec.message());
          CORE_LOG_ERROR("Shutting down because signal handling is compromised.");
          network_system_ptr->get_driver().request_shutdown();
        }
      }

      network_system* network_system_ptr = nullptr;
    };

    static signal_catcher catcher{ this };
    net_context->signals.async_wait(std::bind_front(&signal_catcher::catch_signal, &catcher));

    const bool force_disable_network = get_driver().get_config_value<bool>("networking.force-disable", false);
    if (!force_disable_network) {
      net_context->net_thread = make_scope<network_thread>(net_context->net_thread_message_bus);
      net_context->net_thread->launch();
      net_context->net_thread_message_bus.register_thread();
    }

    initialize_message_handlers();
  }

  void network_system::tick(driver_kernel* kernel, double dt) {
    OTHER_ASSERT(net_context != nullptr, "Network context is not initialized in network system.");
    net_context->io_context.poll();
    if (net_context->io_context.stopped()) {
      net_context->io_context.restart();
    }

    const bool force_disable_network = get_driver().get_config_value<bool>("networking.force-disable", false);
    if (!force_disable_network) {
      auto msg_opt = net_context->net_thread_message_bus.receive_message();
      if (msg_opt.has_value()) {
        process_network_thread_messages(kernel, std::move(*msg_opt));
      }
    }
  }

  void network_system::shutdown(driver_kernel* kernel) {
    OTHER_ASSERT(net_context != nullptr, "Network context is not initialized in network system.");
    message_handlers.clear();

    const bool force_disable_network = get_driver().get_config_value<bool>("networking.force-disable", false);
    if (!force_disable_network) {
      net_context->net_thread->wait_for_shutdown_complete();
      net_context->net_thread = nullptr;
      net_context = nullptr;
    }

    ack_list.clear();
  }

  void network_system::begin_shutdown_sequence(driver_kernel* kernel) {
    net_context->signals.cancel();

    message msg;
    msg.header = {
      .category = COMMAND,
      .id = SHUTDOWN_REQUEST,
    };

    send_message(&get_driver().get_kernel(), std::move(msg));
  }

  void network_system::send_message(driver_kernel* kernel, message&& msg) {
    bool needs_ack = message_requires_acknowledgment(msg.header);
    if (needs_ack) {
      message_handler handler = get_message_handler(msg.header);
      microseconds timeout = get_message_handler_timeout(msg.header);
      natural_t id = send_message_and_wait_acknowledgment(kernel, std::move(msg), timeout, handler);
      CORE_LOG_DEBUG("Sent message {} with acknowledgment ID {}", msg.header, id);
    } else {
      send_to_network_thread(kernel, std::move(msg));
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

  asio::io_context& network_system::io_context() {
    OTHER_ASSERT(net_context != nullptr, "Network context is not initialized in network system.");
    return net_context->io_context;
  }

  message_bus& network_system::net_message_bus() {
    OTHER_ASSERT(net_context != nullptr, "Network context is not initialized in network system.");
    return net_context->net_thread_message_bus;
  }

  bool network_system::network_active() const {
    return net_context->net_thread != nullptr && net_context->net_thread->is_running();
  }

  natural_t network_system::listen_at_endpoint(const binding_point& endpoint) {
    natural_t connection_id = generate_connection_id();
    auto [itr, success] = active_tcp_connections.emplace(connection_id, tcp_connection{ connection_id });
    if (!success) {
      CORE_LOG_ERROR("Failed to create TCP connection for endpoint {}:{}", endpoint.ip, endpoint.port);
      return 0;
    }

    message msg;
    msg.header = {
      .category = COMMAND,
      .id = LISTEN_TCP_CONNECTION,
    };

    listen_tcp_connection_request request{
      .endpoint = endpoint,
      .connection_id = connection_id,
    };
    msg.data = serialize_message(request);

    send_message(&get_driver().get_kernel(), std::move(msg));

    return connection_id;
  }

  void network_system::initialize_message_handlers() {
    {
      auto [itr, success] = message_handlers.insert({
        message_header{ COMMAND, LISTEN_TCP_CONNECTION },
        {
          message_handler{
            [this](message_header h, std::span<const uint8_t> d) { on_ack_listen_at_endpoint(&get_driver().get_kernel(), h, d); },
            [this](message_header h) { on_timeout_listen_at_endpoint(&get_driver().get_kernel(), h); },
          },
        },
      });
      auto [titr, timeout_success] = message_handler_timeouts.insert({ message_header{ COMMAND, LISTEN_TCP_CONNECTION }, seconds(1) });
      OTHER_ASSERT(success, "Failed to insert message handler for LISTEN_TCP_CONNECTION");
    }

    {
      auto [itr, success] = message_handlers.insert({
        message_header{ COMMAND, SHUTDOWN_REQUEST },
        {
          message_handler{
            [this](message_header h, std::span<const uint8_t> d) { on_ack_shutdown_request_network_thread(&get_driver().get_kernel(), h, d); },
            [this](message_header h) { on_timeout_shutdown_request_network_thread(&get_driver().get_kernel(), h); },
          },
        },
      });
      auto [titr, timeout_success] = message_handler_timeouts.insert({ message_header{ COMMAND, SHUTDOWN_REQUEST }, milliseconds(250) });
      OTHER_ASSERT(success, "Failed to insert message handler for SHUTDOWN_REQUEST");
    }
  }

  bool network_system::message_requires_acknowledgment(const message_header& header) const {
    return message_handlers.find(header) != message_handlers.end();
  }

  message_handler network_system::get_message_handler(const message_header& original_header) const {
    auto itr = message_handlers.find(original_header);
    OTHER_ASSERT(itr != message_handlers.end(), "No message handler found for header {}", original_header);
    return itr->second;
  }

  microseconds network_system::get_message_handler_timeout(const message_header& original_header) const {
    auto itr = message_handler_timeouts.find(original_header);
    if (itr != message_handler_timeouts.end()) {
      return itr->second;
    }
    return seconds(1);
  }

  void network_system::send_to_network_thread(driver_kernel* kernel, message&& msg) {
    OTHER_ASSERT(net_context != nullptr, "Network context is not initialized in network system.");
    CORE_LOG_TRACE("[NETWORK SYSTEM TX: {}]", msg.header);
    net_context->net_thread_message_bus.send_message(std::move(msg));
  }

  natural_t network_system::send_message_and_wait_acknowledgment(driver_kernel* kernel, message&& msg, microseconds timeout, message_handler handler) {
    natural_t ack_id = ack_list.register_ack(io_context(), msg.header, timeout, handler);

    message ack_msg;
    ack_msg.header = {
      .category = REQUEST,
      .id = ACK,
    };
    const uint8_t* ack_id_data = reinterpret_cast<const uint8_t*>(&ack_id);
    const uint8_t* header_data = reinterpret_cast<const uint8_t*>(&msg.header);
    ack_msg.data.append_range(std::span(ack_id_data, sizeof(natural_t)));
    ack_msg.data.append_range(std::span(header_data, sizeof(message_header)));
    ack_msg.data.append_range(msg.data);

    send_to_network_thread(kernel, std::move(ack_msg));
    return ack_id;
  }

  void network_system::cancel_acknowledgment(natural_t ack_id) {
    ack_list.cancel_ack(ack_id);
  }

  void network_system::process_network_thread_messages(driver_kernel* kernel, message&& msg) {
    CORE_LOG_TRACE("[NETWORK SYSTEM RX: {}]", msg.header);
    switch (msg.header.category) {
      case NOTIFICATION:
        switch (msg.header.id) {
          case NEW_TCP_CONNECTION_ACCEPTED: handle_notification_new_connection_accepted(kernel, std::move(msg)); break;
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

      case RESPONSE: break;
      case ERROR_ALERT: get_driver().handle_error_alert(std::move(msg)); break;

      default:
        CORE_LOG_ERROR("Server received unknown message category {}", msg.header.category);
        break;
    }
  }

  void network_system::on_ack_listen_at_endpoint(driver_kernel* kernel, message_header header, const std::span<const uint8_t> data) {
    CORE_LOG_DEBUG("Network thread acknowledged listen at endpoint request");
  }

  void network_system::on_timeout_listen_at_endpoint(driver_kernel* kernel, message_header header) {
    CORE_LOG_ERROR("Timeout waiting for network thread to acknowledge listen at endpoint request");
  }

  void network_system::on_ack_shutdown_request_network_thread(driver_kernel* kernel, message_header header, const std::span<const uint8_t> data) {
    OTHER_ASSERT(net_context != nullptr, "Network context is null in driver.");
    OTHER_ASSERT(net_context->net_thread != nullptr, "Network thread is null in driver.");
    CORE_LOG_DEBUG("Network thread acknowledged shutdown request");
    net_context->net_thread->shutdown();
  }

  void network_system::on_timeout_shutdown_request_network_thread(driver_kernel* kernel, message_header header) {
    OTHER_ASSERT(net_context != nullptr, "Network context is null in driver.");
    OTHER_ASSERT(net_context->net_thread != nullptr, "Network thread is null in driver.");
    CORE_LOG_ERROR("Timeout waiting for network thread to acknowledge shutdown request");
    CORE_LOG_ERROR("   Data may be corrupt from unclean shutdown");
    net_context->net_thread->force_shutdown();
  }

  void network_system::handle_notification_new_connection_accepted(driver_kernel* kernel, message&& msg) {
    natural_t connection_id = *reinterpret_cast<const natural_t*>(msg.data.data());
    CORE_LOG_INFO("Network thread accepted new connection with ID {}", connection_id);

    auto [itr, success] = active_tcp_connections.emplace(connection_id, tcp_connection{ connection_id });
    OTHER_ASSERT(success, "Failed to add new TCP connection with ID {} to active connections list", connection_id);

    get_driver().on_new_connection_accepted(connection_id);
  }

  void network_system::handle_notification_network_thread_ready(driver_kernel* kernel, message&& msg) {
    get_driver().confirm_initialization();
  }

  void network_system::handle_notification_network_thread_shutdown_complete(driver_kernel* kernel, message&& msg) {
    get_driver().confirm_network_thread_shutdown();
  }

  void network_system::handle_acknowledgement_ack(driver_kernel* kernel, message&& msg) {
    acknowledgement ack_data = deserialize_message<acknowledgement>(msg.data);
    if (ack_data.ack == 1) {
      CORE_LOG_DEBUG("Received acknowledgment for message {} with ACK ID {}", ack_data.acked_header, ack_data.ack_id);
      ack_list.handle_ack(ack_data.ack_id, ack_data.acked_header, {});
    } else {
      CORE_LOG_WARN("Received failure acknowledgment for message {} with ACK ID {}", ack_data.acked_header, ack_data.ack_id);
      OTHER_ASSERT(false, "Received failure acknowledgment for message {} with ACK ID {}", ack_data.acked_header, ack_data.ack_id);
    }
  }
}  // namespace other
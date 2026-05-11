/**
 * \file driver/systems/network_system.cpp
 **/
#include "driver/systems/network_system.hpp"

#include <cctype>
#include <cstdint>

#include "core/defines.hpp"
#include "core/time.hpp"
#include "thread/thread_safety.hpp"

#include "network/tcp/tcp_transport_provider.hpp"

#include "driver/driver.hpp"

#include "message/message.hpp"
#include "message/messages.hpp"

namespace other {

  void network_system::initialize(driver_kernel* kernel) {
    ASSERT_MAIN_THREAD();
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
    ASSERT_MAIN_THREAD();
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
    ASSERT_MAIN_THREAD();
    OTHER_ASSERT(net_context != nullptr, "Network context is not initialized in network system.");
    message_handlers.clear();

    const bool force_disable_network = get_driver().get_config_value<bool>("networking.force-disable", false);
    if (!force_disable_network) {
      net_context->net_thread->wait_for_shutdown_complete();
      net_context->net_thread = nullptr;
      net_context->registered_transport_providers.clear();
      net_context = nullptr;
    }

    ack_list.clear();
  }

  natural_t network_system::register_transport_provider(scope<transport_provider> provider) {
    ASSERT_MAIN_THREAD();
    OTHER_ASSERT(net_context != nullptr, "Network context is not initialized in network system.");
    OTHER_ASSERT(provider != nullptr, "Cannot register null transport provider.");

    natural_t id = provider->hash();
    if (net_context->registered_transport_providers.find(id) != net_context->registered_transport_providers.end()) {
      CORE_LOG_ERROR("Failed to register transport provider with name '{}', a provider with the same name already exists.", provider->name());
      return 0;
    }

    CORE_LOG_INFO("Registering transport provider '{}' ({:#010x})", provider->name(), id);
    auto [itr, success] = net_context->registered_transport_providers.emplace(id, std::move(provider));
    OTHER_ASSERT(success, "Failed to register transport provider: {}!", itr->second->name());

    net_context->net_thread->register_provider(itr->second.get());

    return id;
  }

  natural_t network_system::register_transport_listener(const std::string_view transport_name, scope<packet_sink> sink) {
    ASSERT_MAIN_THREAD();
    OTHER_ASSERT(net_context != nullptr, "Network context is not initialized in network system.");
    OTHER_ASSERT(sink != nullptr, "Cannot register null packet sink.");

    natural_t id = net_context->generate_packet_sink_id();
    OTHER_ASSERT(net_context->registered_packet_sinks.find(id) == net_context->registered_packet_sinks.end(), "Packet sink ID {} is already in use.", id);

    CORE_LOG_INFO("Registering packet sink [{}]", id);
    auto [itr, success] = net_context->registered_packet_sinks.emplace(id, std::move(sink));
    OTHER_ASSERT(success, "Failed to register packet sink with ID {}!", id);

    uint64_t hash = FNV(
      transport_name |
      std::views::transform([](unsigned char c) { return std::tolower(c); }) |
      std::ranges::to<std::string>()
    );
    net_context->net_thread->register_transport_listener(hash, id, itr->second.get());

    return id;
  }

  natural_t network_system::attach_connection_listener(natural_t connection_id, scope<packet_sink> sink) {
    ASSERT_MAIN_THREAD();
    OTHER_ASSERT(net_context != nullptr, "Network context is not initialized in network system.");
    OTHER_ASSERT(sink != nullptr, "Cannot attach null packet sink.");

    natural_t id = net_context->generate_packet_sink_id();
    OTHER_ASSERT(net_context->registered_packet_sinks.find(id) == net_context->registered_packet_sinks.end(), "Packet sink ID {} is already in use.", id);

    CORE_LOG_INFO("Attaching packet sink [{}] to connection ID {}", id, connection_id);
    auto [itr, success] = net_context->registered_packet_sinks.emplace(id, std::move(sink));
    OTHER_ASSERT(success, "Failed to attach packet sink with ID {} to connection ID {}!", id, connection_id);

    net_context->net_thread->register_packet_sink(id, itr->second.get());
    net_context->net_thread->attach_connection_listener(connection_id, id);

    return connection_id;
  }

  void network_system::attach_connection_listener(natural_t connection_id, natural_t sink_id) {
    ASSERT_MAIN_THREAD();
    OTHER_ASSERT(net_context != nullptr, "Network context is not initialized in network system.");

    if (!net_context->registered_packet_sinks.contains(sink_id)) {
      CORE_LOG_ERROR("Failed to attach packet sink with ID {} to connection ID {}: no such packet sink registered.", sink_id, connection_id);
      return;
    }

    CORE_LOG_INFO("Attaching packet sink [{}] to connection ID {}", sink_id, connection_id);
    // net_context->net_thread->attach_connection_listener(connection_id, sink_id);
  }

  natural_t network_system::listen_at_endpoint(const binding_point& ep, const std::string_view transport_name, natural_t preferred_sink_id) {
    ASSERT_MAIN_THREAD();
    OTHER_ASSERT(net_context != nullptr, "Network context is not initialized in network system.");
    OTHER_ASSERT(net_context->net_thread != nullptr, "Network thread is not initialized in network system.");
    natural_t connection_id = net_context->net_thread->generate_connection_id();
    message msg(COMMAND, LISTEN_CONNECTION);
    command_listen_connection request{
      .endpoint = ep,
      .connection_id = connection_id,
      .transport_hash = FNV(
        transport_name |
        std::views::transform([](unsigned char c) { return std::tolower(c); }) |
        std::ranges::to<std::string>()
      ),
    };
    msg.data = serialize_direct(request);

    send_message(&get_driver().get_kernel(), std::move(msg));

    return connection_id;
  }

  natural_t network_system::connect(const binding_point& ep, const std::string_view transport_name, natural_t preferred_sink_id) {
    ASSERT_MAIN_THREAD();
    // natural_t connection_id = network_thread::generate_connection_id();
    // auto [itr, success] = active_tcp_connections.emplace(connection_id, tcp_connection{ connection_id });
    // if (!success) {
    //   CORE_LOG_ERROR("Failed to create TCP connection for endpoint {}:{}", ep.ip, ep.port);
    //   return 0;
    // }

    // message msg;
    // msg.header = {
    //   .category = COMMAND,
    //   .id = CONNECT_CONNECTION,
    // };
    // command_connect_connection request{
    //   .endpoint = ep,
    //   .connection_id = connection_id,
    //   .transport_hash = FNV(transport_name),
    // };
    // msg.data = serialize_direct(request);

    // send_message(&get_driver().get_kernel(), std::move(msg));

    CORE_LOG_WARN("unimplemented network_system::connect called for endpoint {}:{}", ep.ip, ep.port);
    return 0;
  }

  void network_system::tx_data(natural_t connection_id, std::span<const uint8_t> data) {
    ASSERT_MAIN_THREAD();
    message msg{ COMMAND, TX_DATA };
    command_tx_data tx_data{
      .connection_id = connection_id,
      .data = std::vector(data.begin(), data.end()),
    };
    msg.data = serialize_direct(tx_data);
    send_message(&get_driver().get_kernel(), std::move(msg));
  }

  void network_system::send_message(driver_kernel* kernel, message&& msg) {
    ASSERT_MAIN_THREAD();
    const message_header header = { msg.category, msg.id };
    bool needs_ack = message_requires_acknowledgment(header);
    if (needs_ack) {
      message_handler handler = get_message_handler(header);
      microseconds timeout = get_message_handler_timeout(header);
      natural_t id = send_message_and_wait_acknowledgment(kernel, std::move(msg), timeout, handler);
      CORE_LOG_DEBUG("Sent message {} with acknowledgment ID {}", header, id);
    } else {
      send_to_network_thread(kernel, std::move(msg));
    }
  }

  void network_system::begin_shutdown_sequence(driver_kernel* kernel) {
    ASSERT_MAIN_THREAD();
    net_context->signals.cancel();

    message msg(COMMAND, SHUTDOWN_REQUEST);
    send_message(&get_driver().get_kernel(), std::move(msg));
  }

  void network_system::catch_signal(int signum) {
    ASSERT_MAIN_THREAD();
    if (signum == SIGINT || signum == SIGTERM) {
      CORE_LOG_INFO("Received signal {}, shutting down driver...", signum);
      get_driver().request_shutdown();
    } else {
      CORE_LOG_WARN("Received unhandled signal {}", signum);
    }
  }

  asio::io_context& network_system::io_context() {
    ASSERT_MAIN_THREAD();
    OTHER_ASSERT(net_context != nullptr, "Network context is not initialized in network system.");
    return net_context->io_context;
  }

  message_bus& network_system::net_message_bus() {
    ASSERT_MAIN_THREAD();
    OTHER_ASSERT(net_context != nullptr, "Network context is not initialized in network system.");
    return net_context->net_thread_message_bus;
  }

  bool network_system::network_active() const {
    ASSERT_MAIN_THREAD();
    return net_context->net_thread != nullptr && net_context->net_thread->is_running();
  }

  void network_system::initialize_message_handlers() {
    ASSERT_MAIN_THREAD();
    {
      auto [itr, success] = message_handlers.insert({
        message_header{ COMMAND, LISTEN_CONNECTION },
        {
          message_handler{
            [this](message_header h, std::span<const uint8_t> d) { on_ack_listen_at_endpoint(&get_driver().get_kernel(), h, d); },
            [this](message_header h) { on_timeout_listen_at_endpoint(&get_driver().get_kernel(), h); },
          },
        },
      });
      auto [titr, timeout_success] = message_handler_timeouts.insert({ message_header{ COMMAND, LISTEN_CONNECTION }, seconds(1) });
      OTHER_ASSERT(success, "Failed to insert message handler for LISTEN_CONNECTION");
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
      auto [titr, timeout_success] = message_handler_timeouts.insert({ message_header{ COMMAND, SHUTDOWN_REQUEST }, seconds(5) });
      OTHER_ASSERT(success, "Failed to insert message handler for SHUTDOWN_REQUEST");
    }
  }

  bool network_system::message_requires_acknowledgment(const message_header& header) const {
    ASSERT_MAIN_THREAD();
    return message_handlers.find(header) != message_handlers.end();
  }

  message_handler network_system::get_message_handler(const message_header& original_header) const {
    ASSERT_MAIN_THREAD();
    auto itr = message_handlers.find(original_header);
    OTHER_ASSERT(itr != message_handlers.end(), "No message handler found for header {}", original_header);
    return itr->second;
  }

  microseconds network_system::get_message_handler_timeout(const message_header& original_header) const {
    ASSERT_MAIN_THREAD();
    auto itr = message_handler_timeouts.find(original_header);
    if (itr != message_handler_timeouts.end()) {
      return itr->second;
    }
    return seconds(1);
  }

  void network_system::send_to_network_thread(driver_kernel* kernel, message&& msg) {
    ASSERT_MAIN_THREAD();
    OTHER_ASSERT(net_context != nullptr, "Network context is not initialized in network system.");
    CORE_LOG_TRACE("[NETWORK SYSTEM TX: {}]", message_header{ msg.category, msg.id });
    net_context->net_thread_message_bus.send_message(std::move(msg));
  }

  natural_t network_system::send_message_and_wait_acknowledgment(driver_kernel* kernel, message&& msg, microseconds timeout, message_handler handler) {
    ASSERT_MAIN_THREAD();
    natural_t ack_id = ack_list.register_ack(io_context(), message_header{ msg.category, msg.id }, timeout, handler);

    message ack_msg(REQUEST, ACK);
    request_acknowledgment request_data{
      .ack_id = ack_id,
      .original_header = message_header{ msg.category, msg.id },
      .message_data = std::move(msg.data),
    };
    ack_msg.data = serialize_direct(request_data);

    send_to_network_thread(kernel, std::move(ack_msg));
    return ack_id;
  }

  void network_system::cancel_acknowledgment(natural_t ack_id) {
    ASSERT_MAIN_THREAD();
    ack_list.cancel_ack(ack_id);
  }

  void network_system::process_network_thread_messages(driver_kernel* kernel, message&& msg) {
    ASSERT_MAIN_THREAD();
    CORE_LOG_TRACE("[NETWORK SYSTEM RX: {}]", message_header{ msg.category, msg.id });
    switch (msg.category) {
      case NOTIFICATION:
        switch (msg.id) {
          case NETWORK_THREAD_READY: handle_notification_network_thread_ready(kernel, std::move(msg)); break;
          case NETWORK_THREAD_SHUTDOWN_COMPLETE: handle_notification_network_thread_shutdown_complete(kernel, std::move(msg)); break;
          default:
            CORE_LOG_ERROR("Server received unknown notification message ID {}", msg.id);
            break;
        }
        break;

      case ACKNOWLEDGEMENT:
        switch (msg.id) {
          case ACK: handle_acknowledgement_ack(kernel, std::move(msg)); break;
          default: CORE_LOG_ERROR("Driver received unknown acknowledgment message ID {}", msg.id); break;
        }
        break;

      case CONTROL:
        switch (msg.id) {
          default:
            CORE_LOG_ERROR("Server received unknown CONTROL message ID {}", msg.id);
            break;
        }
        break;

      case COMMAND:
        switch (msg.id) {
          default:
            CORE_LOG_ERROR("Server received unknown COMMAND message ID {}", msg.id);
            break;
        }
        break;

      case REQUEST:
        switch (msg.id) {
          default:
            CORE_LOG_ERROR("Server received unknown REQUEST message ID {}", msg.id);
            break;
        }
        break;

      case RESPONSE: break;
      case ERROR_ALERT:
        CORE_LOG_ERROR("Received error alert from network thread: {}", std::string(msg.data.begin(), msg.data.end()));
        break;

        // case ERROR_ALERT: get_driver().handle_error_alert(std::move(msg)); break;

      default:
        CORE_LOG_ERROR("Server received unknown message category {}", msg.category);
        break;
    }
  }

  void network_system::on_ack_listen_at_endpoint(driver_kernel* kernel, message_header header, const std::span<const uint8_t> data) {
    ASSERT_MAIN_THREAD();
    CORE_LOG_DEBUG("Network thread acknowledged listen at endpoint request");
  }

  void network_system::on_timeout_listen_at_endpoint(driver_kernel* kernel, message_header header) {
    ASSERT_MAIN_THREAD();
    CORE_LOG_ERROR("Timeout waiting for network thread to acknowledge listen at endpoint request");
  }

  void network_system::on_ack_shutdown_request_network_thread(driver_kernel* kernel, message_header header, const std::span<const uint8_t> data) {
    ASSERT_MAIN_THREAD();
    OTHER_ASSERT(net_context != nullptr, "Network context is null in driver.");
    OTHER_ASSERT(net_context->net_thread != nullptr, "Network thread is null in driver.");
    CORE_LOG_DEBUG("Network thread acknowledged shutdown request");
    net_context->net_thread->shutdown();
  }

  void network_system::on_timeout_shutdown_request_network_thread(driver_kernel* kernel, message_header header) {
    ASSERT_MAIN_THREAD();
    OTHER_ASSERT(net_context != nullptr, "Network context is null in driver.");
    OTHER_ASSERT(net_context->net_thread != nullptr, "Network thread is null in driver.");
    CORE_LOG_ERROR("Timeout waiting for network thread to acknowledge shutdown request");
    CORE_LOG_ERROR("   Data may be corrupt from unclean shutdown");
    net_context->net_thread->force_shutdown();
  }

  void network_system::handle_notification_network_thread_ready(driver_kernel* kernel, message&& msg) {
    ASSERT_MAIN_THREAD();
    get_driver().confirm_initialization();
  }

  void network_system::handle_notification_network_thread_shutdown_complete(driver_kernel* kernel, message&& msg) {
    ASSERT_MAIN_THREAD();
    get_driver().confirm_network_thread_shutdown();
  }

  void network_system::handle_acknowledgement_ack(driver_kernel* kernel, message&& msg) {
    ASSERT_MAIN_THREAD();
    acknowledgement_ack ack_data = deserialize_direct<acknowledgement_ack>(msg.data).first;
    if (ack_data.ack == 1) {
      CORE_LOG_DEBUG("Received acknowledgment for message {} with ACK ID {}", ack_data.acked_header, ack_data.ack_id);
      ack_list.handle_ack(ack_data.ack_id, ack_data.acked_header, {});
    } else {
      CORE_LOG_WARN("Received failure acknowledgment for message {} with ACK ID {}", ack_data.acked_header, ack_data.ack_id);
      OTHER_ASSERT(false, "Received failure acknowledgment for message {} with ACK ID {}", ack_data.acked_header, ack_data.ack_id);
    }
  }
}  // namespace other
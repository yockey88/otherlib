/**
 * \file driver/systems/network_system.cpp
 **/
#include "driver/systems/network_system.hpp"

#include <cctype>
#include <cstdint>
#include <thread>

#include "core/defines.hpp"
#include "core/time.hpp"
#include "data-structures/std_container.hpp"
#include "event/event_system.hpp"
#include "thread/thread_safety.hpp"

#include "driver/driver.hpp"

#include "message/message.hpp"
#include "message/messages.hpp"

namespace other {

  void signal_catcher::catch_signal(std::error_code ec, int signum) {
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

  void network_system::initialize(driver_kernel* kernel) {
    ASSERT_MAIN_THREAD();
    PROFILE_SECTION("network_system::initialize");
    net_context = make_scope<network_context>();
    OTHER_ASSERT(net_context != nullptr, "Failed to create network context.");

    net_context->signals.async_wait(std::bind_front(&signal_catcher::catch_signal, &signal_handler));

    network_disabled = get_driver().get_config_value<bool>("networking.force-disable", false);
    if (!network_disabled) {
      net_context->net_thread = make_scope<network_thread>(net_context->net_thread_message_bus);

      net_context->net_thread->launch();
      net_context->net_thread_message_bus.register_thread();

      /// degradation contract: init failure warns once and leaves the context
      ///  UNAVAILABLE — tcp/memory untouched, CI (no client) never notices
      if (get_driver().get_config_value<bool>("steam.enabled", false)) {
        steam_ctx = make_scope<steam_context>();
        steam_ctx->initialize(get_driver().get_config_value<uint32_t>("steam.app-id", 0));
      }
    }

    initialize_message_handlers();
  }

  void network_system::late_initialize(driver_kernel* kernel) {
    ASSERT_MAIN_THREAD();
    PROFILE_SECTION("network_system::late_initialize");

    /// late: this system boots ahead of the event driver system (group 0 vs 1)
    event_system& events = *get_driver().get_event_system();
    events.register_event("network.connection-opened");
    events.register_event("network.connection-closed");
    events.register_event("network.listen-failed");
    events.register_event("network.connect-failed");

    // auto register_interfaces_in_registry = [this](environment_registry& reg) {
    //   reg.register_interface<transport_provider>(
    //     [this](scope<transport_provider> p) { return register_transport_provider(std::move(p)); },
    //     [this](natural_t id) { unregister_transport_provider(id); },
    //     no_args(),  // empty
    //     interface_cardinality::MULTIPLE);

    //   reg.register_interface<packet_sink>(
    //     [this](scope<packet_sink> s, plugin_param_view params) { return register_transport_listener(params.get_or("transport", "invalid"), std::move(s)); },
    //     [this](natural_t id) { unregister_transport_listener(id); },
    //     no_args(),
    //     interface_cardinality::MULTIPLE);
    // };
    // register_interfaces_in_registry(kernel->driver_registry());
    // register_interfaces_in_registry(kernel->project_registry());
  }

  void network_system::tick(driver_kernel* kernel, double dt) {
    ASSERT_MAIN_THREAD();
    OTHER_ASSERT(net_context != nullptr, "Network context is not initialized in network system.");
    PROFILE_SECTION("network_system::tick");

    {
      PROFILE_SECTION("network_system::tick--io_context_poll");
      net_context->io_context.poll();
      if (net_context->io_context.stopped()) {
        net_context->io_context.restart();
      }
    }

    if (!network_disabled) {
      PROFILE_SECTION("network_system::tick--network_thread_messages");

      /// non-blocking drain; a blocking receive costs a ~1ms condvar floor per frame
      while (opt<message> msg_opt = net_context->net_thread_message_bus.try_receive_message()) {
        process_network_thread_messages(kernel, std::move(*msg_opt));
      }
    }

    if (steam_ctx != nullptr) {
      steam_ctx->pump();
    }
  }

  void network_system::shutdown(driver_kernel* kernel) {
    ASSERT_MAIN_THREAD();
    OTHER_ASSERT(net_context != nullptr, "Network context is not initialized in network system.");
    PROFILE_SECTION("network_system::shutdown");
    message_handlers.clear();
    steam_ctx = nullptr;

    if (!network_disabled) {
      net_context->net_thread->wait_for_shutdown_complete();
      net_context->net_thread = nullptr;
      net_context = nullptr;
    }

    ack_list.clear();
  }

  void network_system::wait_for_pump_quiescence(uint64_t recorded_epoch) {
    ASSERT_MAIN_THREAD();
    if (net_context == nullptr || net_context->net_thread == nullptr) {
      return;
    }
    PROFILE_SECTION("network_system::wait_for_pump_quiescence");

    /// epoch > recorded+1: every iteration that could hold the pointer has finished
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(250);
    while (net_context->net_thread->is_running() &&
           net_context->net_thread->reclamation_epoch() <= recorded_epoch + 1) {
      if (std::chrono::steady_clock::now() > deadline) {
        CORE_LOG_WARN("Timed out waiting for network pump quiescence, destroying anyway.");
        break;
      }
      std::this_thread::yield();
    }
  }

  natural_t network_system::listen(const net_address& local, const std::string_view transport_name) {
    ASSERT_MAIN_THREAD();
    OTHER_ASSERT(net_context != nullptr, "Network context is not initialized in network system.");
    PROFILE_SECTION("network_system::listen_at_endpoint");

    if (net_context->net_thread == nullptr) {
      CORE_LOG_WARN("Ignoring listen at {}: networking is disabled.", local.binding);
      return 0;
    }

    return 0;
  }

  natural_t network_system::connect(const net_address& remote, const std::string_view transport_name) {
    ASSERT_MAIN_THREAD();
    OTHER_ASSERT(net_context != nullptr, "Network context is not initialized in network system.");
    PROFILE_SECTION("network_system::connect");

    if (net_context->net_thread == nullptr) {
      CORE_LOG_WARN("Ignoring connect: networking is disabled.");
      return 0;
    }

    return 0;
  }

  void network_system::close(natural_t connection_id) {
    ASSERT_MAIN_THREAD();
    OTHER_ASSERT(net_context != nullptr, "Network context is not initialized in network system.");
    PROFILE_SECTION("network_system::close");

    if (net_context->net_thread == nullptr) {
      return;
    }

    message msg(COMMAND, CLOSE_CONNECTION);
    command_close_connection request{
      .connection_id = connection_id,
      .transport_hash = 0,
    };
    msg.data = serialize_direct(request);
    send_message(&get_driver().get_kernel(), std::move(msg));
  }

  void network_system::tx_data(natural_t connection_id, std::span<const uint8_t> data) {
    ASSERT_MAIN_THREAD();
    message msg{ COMMAND, TX_DATA };
    command_tx_data tx_data{
      .connection_id = connection_id,
      .data = ostd::vector<uint8_t>(data.begin(), data.end()),
    };
    msg.data = serialize_direct(tx_data);
    send_message(&get_driver().get_kernel(), std::move(msg));
  }

  void network_system::send_message(driver_kernel* kernel, message&& msg) {
    ASSERT_MAIN_THREAD();
    PROFILE_SECTION("network_system::send_message");

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

    /// networking disabled: there is no thread to hand a shutdown request to, so
    ///  unblock the driver's shutdown gate immediately
    if (net_context->net_thread == nullptr) {
      get_driver().confirm_network_thread_shutdown();
      return;
    }

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

  network_thread* network_system::thread() {
    ASSERT_MAIN_THREAD();
    return net_context != nullptr ? net_context->net_thread.get() : nullptr;
  }

  void network_system::initialize_message_handlers() {
    ASSERT_MAIN_THREAD();
    PROFILE_SECTION("network_system::initialize_message_handlers");

    {
      auto [itr, success] = message_handlers.insert({
        message_header{ COMMAND, LISTEN_CONNECTION },
        {
          message_handler{
            [this](message_header h, std::span<const uint8_t> d) { on_ack_listen_at_endpoint(&get_driver().get_kernel(), h, d); },
            [this](message_header h) { on_timeout_listen_at_endpoint(&get_driver().get_kernel(), h); },
            [this](message_header h, std::span<const uint8_t> d) { on_failed_listen_at_endpoint(&get_driver().get_kernel(), h, d); },
          },
        },
      });
      auto [titr, timeout_success] = message_handler_timeouts.insert({ message_header{ COMMAND, LISTEN_CONNECTION }, seconds(1) });
      OTHER_ASSERT(success, "Failed to insert message handler for LISTEN_CONNECTION");
    }

    {
      auto [itr, success] = message_handlers.insert({
        message_header{ COMMAND, CONNECT_CONNECTION },
        {
          message_handler{
            [this](message_header h, std::span<const uint8_t> d) { on_ack_connect(&get_driver().get_kernel(), h, d); },
            [this](message_header h) { on_timeout_connect(&get_driver().get_kernel(), h); },
            [this](message_header h, std::span<const uint8_t> d) { on_failed_connect(&get_driver().get_kernel(), h, d); },
          },
        },
      });
      auto [titr, timeout_success] = message_handler_timeouts.insert({ message_header{ COMMAND, CONNECT_CONNECTION }, seconds(3) });
      OTHER_ASSERT(success, "Failed to insert message handler for CONNECT_CONNECTION");
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
    PROFILE_SECTION("network_system::send_to_network_thread");

    CORE_LOG_TRACE("[NETWORK SYSTEM TX: {}]", message_header{ msg.category, msg.id });
    net_context->net_thread_message_bus.send_message(std::move(msg));
  }

  natural_t network_system::send_message_and_wait_acknowledgment(driver_kernel* kernel, message&& msg, microseconds timeout, message_handler handler) {
    ASSERT_MAIN_THREAD();
    PROFILE_SECTION("network_system::send_message_and_wait_acknowledgment");

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

  void network_system::process_network_thread_messages(driver_kernel* kernel, message&& msg) {
    ASSERT_MAIN_THREAD();
    PROFILE_SECTION("network_system::process_network_thread_messages");
    CORE_LOG_TRACE("[NETWORK SYSTEM RX: {}]", message_header{ msg.category, msg.id });
    switch (msg.category) {
      case NOTIFICATION:
        switch (msg.id) {
          case NETWORK_THREAD_READY: handle_notification_network_thread_ready(kernel, std::move(msg)); break;
          case NETWORK_THREAD_SHUTDOWN_COMPLETE: handle_notification_network_thread_shutdown_complete(kernel, std::move(msg)); break;
          case CONNECTION_OPENED: handle_notification_connection_opened(kernel, std::move(msg)); break;
          case CONNECTION_CLOSED: handle_notification_connection_closed(kernel, std::move(msg)); break;
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

  void network_system::on_failed_listen_at_endpoint(driver_kernel* kernel, message_header header, const std::span<const uint8_t> data) {
    ASSERT_MAIN_THREAD();
    CORE_LOG_ERROR("Network thread failed to listen (port in use or no provider)");
    get_driver().get_event_system()->trigger_event("network.listen-failed");
  }

  void network_system::on_ack_connect(driver_kernel* kernel, message_header header, const std::span<const uint8_t> data) {
    ASSERT_MAIN_THREAD();
    CORE_LOG_DEBUG("Network thread acknowledged connect request");
  }

  void network_system::on_timeout_connect(driver_kernel* kernel, message_header header) {
    ASSERT_MAIN_THREAD();
    CORE_LOG_ERROR("Timeout waiting for network thread to acknowledge connect request");
    get_driver().get_event_system()->trigger_event("network.connect-failed");
  }

  void network_system::on_failed_connect(driver_kernel* kernel, message_header header, const std::span<const uint8_t> data) {
    ASSERT_MAIN_THREAD();
    CORE_LOG_ERROR("Network thread failed to start a connect (no such transport provider)");
    get_driver().get_event_system()->trigger_event("network.connect-failed");
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

  void network_system::handle_notification_connection_opened(driver_kernel* kernel, message&& msg) {
    ASSERT_MAIN_THREAD();
    notification_connection_opened data = deserialize_direct<notification_connection_opened>(msg.data).first;
    CORE_LOG_DEBUG("Connection {} opened ({}, remote {}:{})", data.connection_id, data.outbound != 0 ? "outbound" : "inbound", data.remote.ip, data.remote.port);
    get_driver().get_event_system()->trigger_event("network.connection-opened", data.connection_id);
  }

  void network_system::handle_notification_connection_closed(driver_kernel* kernel, message&& msg) {
    ASSERT_MAIN_THREAD();
    notification_connection_closed data = deserialize_direct<notification_connection_closed>(msg.data).first;
    CORE_LOG_DEBUG("Connection {} closed (reason {})", data.connection_id, data.reason);
    get_driver().get_event_system()->trigger_event("network.connection-closed", data.connection_id);
  }

  void network_system::handle_acknowledgement_ack(driver_kernel* kernel, message&& msg) {
    ASSERT_MAIN_THREAD();
    acknowledgement_ack ack_data = deserialize_direct<acknowledgement_ack>(msg.data).first;
    if (ack_data.ack == 1) {
      CORE_LOG_DEBUG("Received acknowledgment for message {} with ACK ID {}", ack_data.acked_header, ack_data.ack_id);
      ack_list.handle_ack(ack_data.ack_id, ack_data.acked_header, {});
    } else {
      /// a failed operation on the network thread is data for the caller, never fatal
      CORE_LOG_WARN("Received failure acknowledgment for message {} with ACK ID {}", ack_data.acked_header, ack_data.ack_id);
      ack_list.handle_failure(ack_data.ack_id, ack_data.acked_header, {});
    }
  }
}  // namespace other
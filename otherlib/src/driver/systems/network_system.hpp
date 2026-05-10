/**
 * \file driver/systems/network_system.hpp
 **/
#ifndef OTHERLIB_DRIVER_SYSTEMS_NETWORK_SYSTEM_HPP
#define OTHERLIB_DRIVER_SYSTEMS_NETWORK_SYSTEM_HPP

#include <asio/asio.hpp>
#include <asio/asio/signal_set.hpp>

#include "core/defines.hpp"
#include "core/time.hpp"
#include "thread/message_bus.hpp"

#include "network/acknowledgement_list.hpp"
#include "network/message_handler.hpp"
#include "network/network_thread.hpp"
#include "network/transport_provider.hpp"

#include "driver/driver_system.hpp"
#include "driver/systems/core_system.hpp"

namespace other {

  class network_system : public core_system<network_system> {
   public:
    enum role {
      SERVER,
      CLIENT,
      NONE,
    };
    struct network_context {
      asio::io_context io_context;
      asio::signal_set signals;
      natural_t netw_thread_heartbeat_timeout_id = 0;
      message_bus net_thread_message_bus;
      scope<network_thread> net_thread = nullptr;
      std::unordered_map<natural_t, scope<transport_provider>> registered_transport_providers;

      constexpr static uint32_t kLocalhostAddress = 0x7f000001;
      constexpr static uint32_t kPrimarySessionBindingPort = 49222;
      constexpr static uint16_t kServerBroadcastPost0 = 50160;
      constexpr static binding_point main_binding_point{
        kLocalhostAddress, kPrimarySessionBindingPort
      };
      uint16_t next_available_server_port = kServerBroadcastPost0;

      network_context() : signals(io_context, SIGINT, SIGTERM) {}
    };

    network_system(driver* driver_instance)
        : core_system(driver_instance, driver_system_type::NETWORK_DRIVER_SYSTEM) {}
    virtual ~network_system() = default;

    std::string name() const override { return "Network System"; }

    void initialize(driver_kernel* kernel) override;
    void tick(driver_kernel* kernel, double dt) override;
    void shutdown(driver_kernel* kernel) override;

    natural_t register_transport_provider(scope<transport_provider> provider);

    void set_default_packet_sink(packet_sink* sink);

    natural_t listen_at_endpoint(const binding_point& ep, const std::string_view transport_name = "tcp", packet_sink* sink = nullptr);
    natural_t connect(const binding_point& ep, const std::string_view transport_name = "tcp", packet_sink* sink = nullptr);

    void tx_data(natural_t connection_id, std::span<const uint8_t> data);

    void send_message(driver_kernel* kernel, message&& msg);
    void begin_shutdown_sequence(driver_kernel* kernel);

    void catch_signal(int signal);

    asio::io_context& io_context();
    message_bus& net_message_bus();

    bool network_active() const;

    natural_t listen_at_endpoint(const binding_point& endpoint);

   private:
    struct tcp_connection {
      natural_t connection_id;
    };

    std::map<natural_t, tcp_connection> active_tcp_connections;

    scope<network_context> net_context = nullptr;
    acknowledgement_list ack_list;

    std::map<message_header, message_handler> message_handlers;
    std::map<message_header, microseconds> message_handler_timeouts;

    void initialize_message_handlers();
    bool message_requires_acknowledgment(const message_header& header) const;
    message_handler get_message_handler(const message_header& original_header) const;
    microseconds get_message_handler_timeout(const message_header& original_header) const;

    void send_to_network_thread(driver_kernel* kernel, message&& msg);
    natural_t send_message_and_wait_acknowledgment(driver_kernel* kernel, message&& msg, microseconds timeout, message_handler handler);
    void cancel_acknowledgment(natural_t ack_id);

    void process_network_thread_messages(driver_kernel* kernel, message&& msg);

    /// ack/timeout callbacks
    void on_ack_listen_at_endpoint(driver_kernel* kernel, message_header header, const std::span<const uint8_t> data);
    void on_timeout_listen_at_endpoint(driver_kernel* kernel, message_header header);

    void on_ack_shutdown_request_network_thread(driver_kernel* kernel, message_header header, const std::span<const uint8_t> data);
    void on_timeout_shutdown_request_network_thread(driver_kernel* kernel, message_header header);

    // response/timeout callbacks
    /// message handlers
    /// notifications
    void handle_notification_network_thread_ready(driver_kernel* kernel, message&& msg);
    void handle_notification_network_thread_shutdown_complete(driver_kernel* kernel, message&& msg);
    void handle_notification_rx_data(driver_kernel* kernel, message&& msg);
    void handle_notification_connect_tcp_connection(driver_kernel* kernel, message&& msg);
    void handle_notification_close_tcp_connection(driver_kernel* kernel, message&& msg);

    /// acknowledgments
    void handle_acknowledgement_ack(driver_kernel* kernel, message&& msg);
    /// control messages
    /// command messages
    /// request messages
    /// response messages
  };

}  // namespace other

#endif  // OTHERLIB_DRIVER_SYSTEMS_NETWORK_SYSTEM_HPP
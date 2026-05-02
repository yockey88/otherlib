/**
 * \file driver/systems/network_system.hpp
 **/
#ifndef OTHERLIB_DRIVER_SYSTEMS_NETWORK_SYSTEM_HPP
#define OTHERLIB_DRIVER_SYSTEMS_NETWORK_SYSTEM_HPP

#include <asio/asio.hpp>
#include <asio/asio/signal_set.hpp>

#include "thread/message_bus.hpp"

#include "network/message_handler.hpp"
#include "network/network_thread.hpp"

#include "driver/acknowledgement_list.hpp"
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

    network_system(driver* driver_instance)
        : core_system(driver_instance, driver_system_type::NETWORK_DRIVER_SYSTEM) {}
    virtual ~network_system() = default;

    std::string name() const override { return "Network System"; }

    void initialize(driver_kernel* kernel) override;
    void tick(driver_kernel* kernel, double dt) override;
    void shutdown(driver_kernel* kernel) override;

    void begin_shutdown_sequence(driver_kernel* kernel);

    void send_to_network_thread(driver_kernel* kernel, message&& msg);

    natural_t open_tcp_connection(const binding_point& endpoint);

    void send_message_and_detach_acknowledgement(driver_kernel* kernel, message&& msg, message_handler handler);
    natural_t send_message_and_wait_acknowledgment(driver_kernel* kernel, message&& msg, microseconds timeout, message_handler handler);
    void cancel_acknowledgment(natural_t ack_id);

    void catch_signal(int signal);

    asio::io_context& io_context();
    message_bus& net_message_bus();

    bool network_active() const;

   private:
    struct network_context {
      asio::io_context io_context;
      asio::signal_set signals;
      natural_t netw_thread_heartbeat_timeout_id = 0;
      message_bus net_thread_message_bus;
      scope<network_thread> net_thread = nullptr;

      constexpr static uint32_t kLocalhostAddress = 0x7f000001;
      constexpr static uint32_t kPrimarySessionBindingPort = 49222;
      constexpr static uint16_t kServerBroadcastPost0 = 50160;
      constexpr static binding_point main_binding_point{
        kLocalhostAddress, kPrimarySessionBindingPort
      };
      uint16_t next_available_server_port = kServerBroadcastPost0;

      network_context() : signals(io_context, SIGINT, SIGTERM) {}
    };

    scope<network_context> net_context = nullptr;
    acknowledgement_list ack_list;

    void process_network_thread_messages(driver_kernel* kernel, message&& msg);

    /// ack/timeout callbacks
    void on_ack_shutdown_request_network_thread(driver_kernel* kernel, message_header header, const std::span<const uint8_t> data);
    void on_timeout_shutdown_request_network_thread(driver_kernel* kernel, message_header header);

    // response/timeout callbacks
    /// message handlers
    /// notifications
    void handle_notification_network_thread_ready(driver_kernel* kernel, message&& msg);
    void handle_notification_network_thread_shutdown_complete(driver_kernel* kernel, message&& msg);

    /// acknowledgments
    void handle_acknowledgement_ack(driver_kernel* kernel, message&& msg);
    /// control messages
    /// command messages
    /// request messages
    /// response messages
  };

}  // namespace other

#endif  // OTHERLIB_DRIVER_SYSTEMS_NETWORK_SYSTEM_HPP
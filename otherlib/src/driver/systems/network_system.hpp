/**
 * \file driver/systems/network_system.hpp
 **/
#ifndef OTHERLIB_DRIVER_SYSTEMS_NETWORK_SYSTEM_HPP
#define OTHERLIB_DRIVER_SYSTEMS_NETWORK_SYSTEM_HPP

#include <asio/asio.hpp>
#include <asio/asio/signal_set.hpp>

#include "core/defines.hpp"
#include "core/time.hpp"
#include "data-structures/std_container.hpp"

#include "network/acknowledgement_list.hpp"
#include "network/net_address.hpp"
#include "network/network_thread.hpp"
#include "network/transport_provider.hpp"

#include "steam/steam_context.hpp"

#include "driver/driver_system.hpp"
#include "driver/systems/core_system.hpp"

#include "message/message.hpp"
#include "message/message_bus.hpp"
#include "peer_mesh/packet_sink.hpp"

namespace other {

  class network_system;

  struct signal_catcher {
    signal_catcher(network_system* network_system_ptr)
        : network_system_ptr(network_system_ptr) {}

    void catch_signal(std::error_code ec, int signum);

    network_system* network_system_ptr = nullptr;
  };

  class OTHER_CLASS network_system : public core_system<network_system> {
   public:
    enum role {
      SERVER,
      CLIENT,
      NONE,
    };
    struct network_context {
      asio::io_context io_context;
      asio::signal_set signals;
      message_bus net_thread_message_bus;
      scope<network_thread> net_thread = nullptr;

      ostd::unordered_map<natural_t, scope<packet_sink>> registered_packet_sinks;
      ostd::unordered_map<natural_t, scope<transport_provider>> registered_transport_providers;
      /// providers whose posted net-io teardown never confirmed; destroyed only after
      ///  the network thread has joined
      ostd::vector<scope<transport_provider>> orphaned_transport_providers;

      constexpr static uint32_t kLocalhostAddress = 0x7f000001;
      constexpr static uint32_t kPrimarySessionBindingPort = 49222;
      constexpr static uint16_t kServerBroadcastPost0 = 50160;
      constexpr static binding_point main_binding_point{
        kLocalhostAddress, kPrimarySessionBindingPort
      };
      uint16_t next_available_server_port = kServerBroadcastPost0;

      network_context() : signals(io_context, SIGINT, SIGTERM) {}

      inline natural_t generate_packet_sink_id() {
        return packet_sink_id_counter++;
      }

     private:
      natural_t packet_sink_id_counter = 1;
    };

    network_system(driver* driver_instance)
        : core_system(driver_instance, driver_system_type::NETWORK_DRIVER_SYSTEM) {}
    virtual ~network_system() = default;

    std::string name() const override { return "Network System"; }

    void initialize(driver_kernel* kernel) override;
    void late_initialize(driver_kernel* kernel) override;
    void tick(driver_kernel* kernel, double dt) override;
    void shutdown(driver_kernel* kernel) override;

    natural_t register_transport_provider(scope<transport_provider> provider);
    void unregister_transport_provider(natural_t provider_id);

    natural_t register_transport_listener(const std::string_view transport_name, scope<packet_sink> sink);
    void unregister_transport_listener(natural_t sink_id);

    natural_t listen_at_endpoint(const binding_point& ep, const std::string_view transport_name = "tcp");
    /// dials by kind-tagged address (empty transport resolves via kind, e.g. IP->tcp);
    ///  returns conn id or 0 if refused. completion arrives via connection-opened/closed events
    natural_t connect(const net_address& remote, const std::string_view transport_name = "");
    void close(natural_t connection_id);

    void tx_data(natural_t connection_id, std::span<const uint8_t> data);

    void send_message(driver_kernel* kernel, message&& msg);
    void begin_shutdown_sequence(driver_kernel* kernel);

    void catch_signal(int signal);

    asio::io_context& io_context();
    message_bus& net_message_bus();

    bool network_active() const;

    /// peer-mesh glue: exposes the net thread/providers for link-transport adapters to
    ///  wrap, plus the only feed for accept attribution and dial failures
    network_thread* thread();
    transport_provider* find_provider(const std::string_view transport_name);
    void set_connection_taps(std::function<void(const notification_connection_opened&)> on_open,
                             std::function<void(const notification_connection_closed&)> on_close);

    /// nullptr when steam.enabled is false; group-0 tick order pumps its callbacks
    ///  before peer_mesh_system ticks the mesh
    steam_context* steam() { return steam_ctx.get(); }

   private:
    signal_catcher signal_handler{ this };

    scope<network_context> net_context = nullptr;
    /// networking.force-disable, read once at init
    bool network_disabled = false;
    acknowledgement_list ack_list;
    scope<steam_context> steam_ctx = nullptr;

    std::function<void(const notification_connection_opened&)> connection_opened_tap;
    std::function<void(const notification_connection_closed&)> connection_closed_tap;

    /// bounded wait until the pump epoch proves no reader holds an unregistered pointer —
    ///  providers/sinks can live in plugins that unload the moment unregister returns
    void wait_for_pump_quiescence(uint64_t recorded_epoch);

    ostd::map<message_header, message_handler> message_handlers;
    ostd::map<message_header, microseconds> message_handler_timeouts;

    void initialize_message_handlers();
    bool message_requires_acknowledgment(const message_header& header) const;
    message_handler get_message_handler(const message_header& original_header) const;
    microseconds get_message_handler_timeout(const message_header& original_header) const;

    void send_to_network_thread(driver_kernel* kernel, message&& msg);
    natural_t send_message_and_wait_acknowledgment(driver_kernel* kernel, message&& msg, microseconds timeout, message_handler handler);

    void process_network_thread_messages(driver_kernel* kernel, message&& msg);

    /// ack/timeout/failure callbacks
    void on_ack_listen_at_endpoint(driver_kernel* kernel, message_header header, const std::span<const uint8_t> data);
    void on_timeout_listen_at_endpoint(driver_kernel* kernel, message_header header);
    void on_failed_listen_at_endpoint(driver_kernel* kernel, message_header header, const std::span<const uint8_t> data);

    void on_ack_connect(driver_kernel* kernel, message_header header, const std::span<const uint8_t> data);
    void on_timeout_connect(driver_kernel* kernel, message_header header);
    void on_failed_connect(driver_kernel* kernel, message_header header, const std::span<const uint8_t> data);

    void on_ack_shutdown_request_network_thread(driver_kernel* kernel, message_header header, const std::span<const uint8_t> data);
    void on_timeout_shutdown_request_network_thread(driver_kernel* kernel, message_header header);

    /// message handlers (notifications below)
    void handle_notification_network_thread_ready(driver_kernel* kernel, message&& msg);
    void handle_notification_network_thread_shutdown_complete(driver_kernel* kernel, message&& msg);
    void handle_notification_connection_opened(driver_kernel* kernel, message&& msg);
    void handle_notification_connection_closed(driver_kernel* kernel, message&& msg);
    /// acknowledgments
    void handle_acknowledgement_ack(driver_kernel* kernel, message&& msg);
  };

}  // namespace other

#endif  // OTHERLIB_DRIVER_SYSTEMS_NETWORK_SYSTEM_HPP
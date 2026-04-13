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
#include "driver/application_list.hpp"
#include "driver/driver_kernel.hpp"
#include "driver/response_list.hpp"
#include "driver/systems/driver_system.hpp"
#include "driver/timer_list.hpp"

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

    void start_network(driver_kernel* kernel);
    void begin_shutdown_sequence(driver_kernel* kernel);

    void send_to_network_thread(driver_kernel* kernel, message&& msg);

    void send_message_and_detach_acknowledgement(driver_kernel* kernel, message&& msg, message_handler handler);
    natural_t send_message_and_wait_acknowledgment(driver_kernel* kernel, message&& msg, microseconds timeout, message_handler handler);
    void cancel_acknowledgment(natural_t ack_id);

    void send_message_and_detach_response(driver_kernel* kernel, message&& msg, message_handler handler);
    natural_t send_message_and_wait_response(driver_kernel* kernel, message&& msg, microseconds timeout, message_handler handler);
    void cancel_response(natural_t response_id);

    void catch_signal(int signal);

    natural_t set_timeout(microseconds duration, timer_list::timeout::on_timeout timeout_callback);
    void clear_timeout(natural_t timeout_id);

    asio::io_context& io_context();
    message_bus& net_message_bus();
    bool network_active() const;
    role get_role() const { return primary_role; }
    opt<integer_t> primary_session_id() const { return client_session_id; }
    application_list& registered_applications() { return app_list; }

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
    struct open_stream {
      integer_t stream_id = 0;
      // ...
    };

    scope<network_context> net_context = nullptr;
    acknowledgement_list ack_list;
    response_list resp_list;
    timer_list timeout_list;
    application_list app_list;

    role primary_role = CLIENT;
    opt<integer_t> client_session_id;

    std::vector<open_stream> active_streams;

    void register_other_application(driver_kernel* kernel, integer_t session_id, application_list::other_application* app);
    void request_scene_udp_binding(driver_kernel* kernel, udp_binding_information address);

    void process_network_thread_messages(driver_kernel* kernel, message&& msg);

    /// ack/timeout callbacks
    void on_ack_command_environment_load_scene(driver_kernel* kernel, message_header header, std::span<const uint8_t> data);
    void on_timeout_environment_load_scene(driver_kernel* kernel, message_header header);
    void on_ack_shutdown_request_network_thread(driver_kernel* kernel, message_header header, const std::span<const uint8_t> data);
    void on_timeout_shutdown_request_network_thread(driver_kernel* kernel, message_header header);
    void on_ack_session_connect_to(driver_kernel* kernel, message_header header, const std::span<const uint8_t> data);
    void on_timeout_session_connect_to(driver_kernel* kernel, message_header header);
    void on_ack_session_listen_for_network_thread(driver_kernel* kernel, message_header header, const std::span<const uint8_t> data);
    void on_timeout_session_listen_for_network_thread(driver_kernel* kernel, message_header header);
    // response/timeout callbacks
    void on_respond_session_check_in(driver_kernel* kernel, message_header header, const std::span<const uint8_t> data);
    void on_respond_new_udp_stream_binding(driver_kernel* kernel, message_header header, std::span<const uint8_t> data);
    void on_timeout_new_udp_stream_binding(driver_kernel* kernel, message_header header);

    /// message handlers
    /// notifications
    void handle_notification_stream_receive_udp_datagram(driver_kernel* kernel, message&& msg);
    void handle_notification_session_check_in(driver_kernel* kernel, message&& msg);
    void handle_notification_session_closed(driver_kernel* kernel, message&& msg);

    /// acknowledgments
    void handle_acknowledgement_ack(driver_kernel* kernel, message&& msg);

    /// control messages
    void handle_control_ping(driver_kernel* kernel, message&& msg);
    void handle_control_pong(driver_kernel* kernel, message&& msg);

    /// command messages
    void handle_command_environment_load_scene(driver_kernel* kernel, integer_t session_id, message&& msg);

    /// request messages
    void handle_request_session_information(driver_kernel* kernel, integer_t session_id, message&& msg);
    void session_check_in_request(driver_kernel* kernel, integer_t session_id);
    void session_application_information_request(driver_kernel* kernel, integer_t session_id, application_list::other_application* app = nullptr);

    /// response messages
    void handle_response(driver_kernel* kernel, message&& msg);
    void handle_response_session_information(driver_kernel* kernel, integer_t session_id, message&& msg);

    /// session events
    void print_session_information(driver_kernel* kernel, application_list::other_application* app);
    void handle_session_event_rx_message(driver_kernel* kernel, message&& msg);
    void handle_session_information_response(driver_kernel* kernel, integer_t session_id, session_information_response&& response);
  };

}  // namespace other

#endif  // OTHERLIB_DRIVER_SYSTEMS_NETWORK_SYSTEM_HPP
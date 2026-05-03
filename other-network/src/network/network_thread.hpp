/**
 * \file network/network_thread.hpp
 **/
#ifndef OTHER_NETWORK_NETWORK_NETWORK_THREAD_HPP
#define OTHER_NETWORK_NETWORK_NETWORK_THREAD_HPP

#include <asio/asio.hpp>

#include "thread/message.hpp"
#include "thread/message_bus.hpp"
#include "thread/thread.hpp"

#include "connection/connection.hpp"
#include "network/io.hpp"

#include "acknowledgement_list.hpp"

namespace other {

  class network_thread : public thread {
   public:
    network_thread(message_bus& bus)
        : thread("OtherServer-Network-Thread"),
          bus(bus), network_io{}, events(network_io.context) {}
    virtual ~network_thread() = default;

    void receive_data(natural_t connection_id, const std::span<uint8_t> data);

    inline message_bus& get_message_bus() { return bus; }
    inline asio::io_context& get_io_context() { return network_io.context; }

   protected:
    message_bus& bus;

    struct state {
      std::mutex mutex;

      natural_t shutdown_ack_id = 0;
      bool shutdown_pending = false;
      bool shutdown_complete = false;
    };
    state current_state{};

    io network_io;
    event_system events;

    natural_t listener_id_counter = 1;
    natural_t connection_id_counter = 1;
    std::map<natural_t, scope<asio::ip::tcp::acceptor>> active_tcp_listeners;
    std::map<natural_t, scope<connection>> active_connections;

    acknowledgement_list ack_list;

    inline natural_t generate_listener_id() { return listener_id_counter++; }
    inline natural_t generate_connection_id() { return connection_id_counter++; }

    void send_to_driver(message&& msg);

    void on_initialize() override;
    void on_start() override;
    void on_shutdown() override;

    void pump_thread() override;
    void process_message(opt<message>&& msg);

    void accept_tcp_connection(asio::ip::tcp::socket socket, const binding_point& endpoint);
    void finalize_connection_establishment(natural_t connection_id);

    void handle_control_ping(message&& msg);
    void handle_command_shutdown_request(message&& msg);
    void handle_command_listen_tcp_connection(message&& msg);
    void handle_command_connect_tcp_connection(message&& msg);
    void handle_command_open_udp_connection(message&& msg);
    void handle_command_close_connection(message&& msg);

    bool immediately_acknowledge_message(const message_header& header);
    void handle_request_ack_process_msg(message&& msg);

    static inline natural_t max_connections = 1024;
    natural_t current_connections = 0;

    microseconds get_message_timeout() override {
      return microseconds(10);
    }
  };

}  // namespace other

#endif  // OTHER_NETWORK_THREAD_HPP

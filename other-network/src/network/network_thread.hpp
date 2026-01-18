/**
 * \file network/network_thread.hpp
 **/
#ifndef OTHER_NETWORK_NETWORK_NETWORK_THREAD_HPP
#define OTHER_NETWORK_NETWORK_NETWORK_THREAD_HPP

#include <thread>

#include <asio/asio.hpp>

#include "thread/message.hpp"
#include "thread/message_bus.hpp"
#include "thread/thread.hpp"

#include "network/session.hpp"
#include "network/udp_stream.hpp"

namespace other {

  class network_thread : public thread {
   public:
    network_thread(message_bus& bus)
        : thread("OtherServer-Network-Thread"),
          bus(bus), net_context{ std::make_unique<network_context>() } {}
    virtual ~network_thread() = default;

    void report_connection_closed(natural_t connection_id, integer_t session_id);
    void report_connection_error(session* cli, const asio::error_code& ec);

    void report_stream_closed(natural_t connection_id, integer_t stream_id);
    void report_stream_error(udp_stream* strm, const asio::error_code& ec);

    void report_connection_check_in_begin(natural_t connection_id, integer_t session_id);
    void report_connection_check_in(natural_t connection_id, integer_t session_id);

    inline integer_t get_next_session_id() {
      static integer_t next_id = 1;
      return next_id++;
    }

    message_bus& get_message_bus() {
      return bus;
    }

    asio::io_context& get_io_context() {
      return net_context->io_context;
    }

    struct connection_key {
      natural_t connection_number = 0;
      integer_t id = 0;
    };

   protected:
    message_bus& bus;

    struct state {
      bool shutdown_pending = false;
      bool shutdown_complete = false;
    };
    state current_state{};

    struct network_context {
      asio::io_context io_context;
      asio::ip::tcp::acceptor acceptor;

      network_context()
          : acceptor(io_context) {}
    };
    std::unique_ptr<network_context> net_context = nullptr;

    struct connection {
      connection_key connection_id;
      binding_point endpoint;
      scope<session> active_session = nullptr;
    };
    std::deque<connection> pending_connections;
    std::unordered_map<integer_t, connection> client_endpoints;

    using event_callback = std::function<void(integer_t)>;
    std::unordered_map<integer_t, event_callback> check_in_listeners;

    struct udp_binding {
      connection_key connection_id;
      binding_point endpoint;

      scope<udp_stream> stream = nullptr;
    };
    uint16_t next_local_udp_port = 60000;
    integer_t next_udp_binding_id = 1;
    std::map<natural_t, udp_binding> udp_bindings;

    std::queue<connection_key> session_closures;
    std::queue<connection_key> stream_closures;

    void handle_session_closures();
    void handle_stream_closures();

    void handle_session_closed(connection_key session_id);
    void handle_stream_closed(connection_key stream_id);

    void on_initialize() override;
    void on_start() override;
    void on_shutdown() override;

    void pump_thread() override;

    void accept_connections(asio::ip::tcp::socket&& socket, const asio::error_code& ec);

    void open_session_and_check_in_at(integer_t session_id, uint16_t port);
    void open_session_and_connect_to(const binding_point& bp);

    void handle_control_ping(message&& msg);

    void handle_command_shutdown_request(message&& msg);
    void handle_command_session_listen_for(message&& msg);
    void handle_command_session_connect_to(message&& msg);
    void handle_command_session_check_in(message&& msg);
    void handle_command_session_tx_message(message&& msg);
    void handle_command_stream_send_udp_datagram(message&& msg);
    void handle_command_environment_load_scene(message&& msg);
    void on_acknowledge_environment_load_scene(message&& msg);

    void handle_request_session_check_in(message&& msg);
    void handle_request_new_udp_stream_binding(message&& msg);

    static inline natural_t max_connections = 1024;
    natural_t current_connections = 0;
  };

}  // namespace other

namespace std {

  template <>
  struct formatter<other::network_thread::connection_key> : public formatter<std::string_view> {
    auto format(const other::network_thread::connection_key& key, format_context& ctx) const {
      return formatter<std::string_view>::format(std::format("[{},{}]", key.connection_number, key.id), ctx);
    }
  };

}  // namespace std

#endif  // OTHER_NETWORK_THREAD_HPP

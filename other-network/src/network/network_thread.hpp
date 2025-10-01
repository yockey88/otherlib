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

namespace other {

  class network_thread : public thread {
   public:
    network_thread(message_bus& bus)
        : thread("OtherServer-Network-Thread"),
          bus(bus), net_context{ std::make_unique<network_context>() } {}
    virtual ~network_thread() = default;

    void report_connection_closed(natural_t client_id);
    void report_connection_error(session* cli, const asio::error_code& ec);

    void report_connection_check_in(natural_t connection_id, integer_t session_id);

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
      natural_t connection_number = 0;
      integer_t session_id = 0;
      binding_point endpoint;
      scope<session> active_session = nullptr;
    };
    std::deque<connection> pending_connections;
    std::unordered_map<integer_t, connection> client_endpoints;

    using event_callback = std::function<void(integer_t)>;
    std::unordered_map<integer_t, event_callback> check_in_listeners;

    void on_initialize() override;
    void on_start() override;
    void on_shutdown() override;

    void pump_thread() override;

    void accept_connections(asio::ip::tcp::socket&& socket, const asio::error_code& ec);

    void open_session_and_check_in_at(integer_t session_id, uint16_t port);

    void handle_control_ping(message&& msg);

    void handle_command_shutdown_request(message&& msg);
    void handle_command_session_check_in(message&& msg);
    void handle_command_session_listen_for(message&& msg);

    void handle_request_session_check_in(message&& msg);

    static inline natural_t max_connections = 1024;
    natural_t current_connections = 0;

    inline integer_t get_next_connection_id() {
      static integer_t next_id = 1;
      return next_id++;
    }
  };

}  // namespace other

#endif  // OTHER_NETWORK_THREAD_HPP

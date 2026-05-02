/**
 * \file network/network_thread.hpp
 **/
#ifndef OTHER_NETWORK_NETWORK_NETWORK_THREAD_HPP
#define OTHER_NETWORK_NETWORK_NETWORK_THREAD_HPP

#include <asio/asio.hpp>

#include "thread/message.hpp"
#include "thread/message_bus.hpp"
#include "thread/thread.hpp"

namespace other {

  class network_thread : public thread {
   public:
    network_thread(message_bus& bus)
        : thread("OtherServer-Network-Thread"),
          bus(bus), net_context{} {}
    virtual ~network_thread() = default;

    inline message_bus& get_message_bus() { return bus; }
    inline asio::io_context& get_io_context() { return net_context->io_context; }

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

    void on_initialize() override;
    void on_start() override;
    void on_shutdown() override;

    void pump_thread() override;

    void accept_connections(asio::ip::tcp::socket&& socket, const asio::error_code& ec);

    void handle_control_ping(message&& msg);
    void handle_command_shutdown_request(message&& msg);

    static inline natural_t max_connections = 1024;
    natural_t current_connections = 0;
  };

}  // namespace other

#endif  // OTHER_NETWORK_THREAD_HPP

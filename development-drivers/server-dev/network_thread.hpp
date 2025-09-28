/**
 * \file server-dev/network_thread.hpp
 **/
#ifndef OTHER_NETWORK_THREAD_HPP
#define OTHER_NETWORK_THREAD_HPP

#include <thread>

#include <asio/asio.hpp>

#include "thread/message.hpp"
#include "thread/thread.hpp"

#include "other_client.hpp"
#include "util/message_passing.hpp"

namespace other {

  class network_thread : public thread {
   public:
    network_thread(asio::io_context& io_context, signals::bus& bus, const binding_point& binding)
        : thread("OtherServer-Network-Thread"), io_context(io_context),
          message_bus(bus), binding(binding), net_context{ std::make_unique<network_context>(io_context) } {}
    virtual ~network_thread() = default;

    void report_connection_closed(natural_t client_id);
    void report_connection_error(client* cli, const asio::error_code& ec);

   protected:
    asio::io_context& io_context;
    signals::bus& message_bus;

    binding_point binding;
    struct network_context {
      asio::io_context& io_context;
      asio::ip::tcp::acceptor acceptor;

      network_context(asio::io_context& io_ctx)
          : io_context(io_ctx),
            acceptor(io_ctx) {}
    };
    std::unique_ptr<network_context> net_context = nullptr;

    struct connections {
      uint64_t session_id = 0;
      binding_point endpoint;
      client connection;
    };
    std::unordered_map<uint64_t, connections> client_endpoints;

    void on_initialize() override;
    void on_start() override;
    void on_shutdown() override;

    void pump_thread() override;

    void accept_connections(asio::ip::tcp::socket&& socket, const asio::error_code& ec);

    static inline natural_t max_connections = 1024;
    natural_t current_connections = 0;

    inline natural_t get_next_connection_id() {
      static natural_t next_id = 1;
      return next_id++;
    }
  };

}  // namespace other

#endif  // OTHER_NETWORK_THREAD_HPP

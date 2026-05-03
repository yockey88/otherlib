/**
 * \file connection/connection.hpp
 **/
#ifndef OTHER_NETWORK_CONNECTION_CONNECTION_HPP
#define OTHER_NETWORK_CONNECTION_CONNECTION_HPP

#include "core/async_buffer.hpp"
#include "event/event_system.hpp"
#include "thread/message.hpp"

#include "connection/connection_state_maching.hpp"
#include "network/io.hpp"

namespace other {

  class network_thread;

  class connection {
   public:
    connection(network_thread* thread, natural_t id, event_system& events, io& io_context, const binding_point& endpoint, asio::ip::tcp::socket tcp_socket)
        : parent_thread(thread), id(id), events(events), io_context(io_context), local_endpoint(endpoint) {
      conn.tcp_socket = make_scope<asio::ip::tcp::socket>(std::move(tcp_socket));
    }
    connection(network_thread* thread, natural_t id, event_system& events, io& io_context, const binding_point& endpoint, asio::ip::udp::socket udp_socket)
        : parent_thread(thread), id(id), events(events), io_context(io_context), local_endpoint(endpoint) {
      conn.udp_socket = make_scope<asio::ip::udp::socket>(std::move(udp_socket));
    }
    virtual ~connection() = default;

    static scope<connection> create_tcp_connection(network_thread* thread, natural_t id, event_system& events, io& io_context, const binding_point& endpoint, asio::ip::tcp::socket tcp_socket);
    static scope<connection> create_udp_connection(network_thread* thread, natural_t id, event_system& events, io& io_context, const binding_point& endpoint, asio::ip::udp::socket udp_socket);

    void poll();
    void shutdown();

    void start_read();
    void write(const std::vector<uint8_t>& data);

   private:
    struct socket {
      scope<asio::ip::tcp::socket> tcp_socket = nullptr;
      scope<asio::ip::udp::socket> udp_socket = nullptr;
    };

    network_thread* parent_thread = nullptr;
    socket conn;

    constexpr static size_t kMaxReadBufferSize = 8192;
    async_buffer<kMaxReadBufferSize> buffer;

    natural_t id;

    event_system& events;
    io& io_context;

    binding_point local_endpoint;

    void finish_read(const asio::error_code& ec, size_t bytes_transferred);
    void finish_write(const asio::error_code& ec, size_t bytes_transferred);
  };

}  // namespace other

#endif  // OTHER_NETWORK_CONNECTION_CONNECTION_HPP
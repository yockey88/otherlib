/**
 * \file network/connection.hpp
 **/
#ifndef OTHER_NETWORK_NETWORK_CONNECTION_HPP
#define OTHER_NETWORK_NETWORK_CONNECTION_HPP

#include "core/async_buffer.hpp"
#include "thread/message.hpp"

#include "network/connection_state_maching.hpp"
#include "network/io.hpp"

namespace other {

  class connection {
   public:
    connection(io& io_context, const binding_point& endpoint)
        : io_context(io_context), conn(this), endpoint(endpoint) {}
    virtual ~connection() = default;

    static scope<connection> tcp_connection(io& io_context, const binding_point& endpoint);
    static scope<connection> udp_connection(io& io_context, const binding_point& endpoint);

    void listen_on_tcp_endpoint(const binding_point& endpoint);
    void connect_to_tcp_endpoint(const binding_point& endpoint);

    void open_udp_endpoint(const binding_point& endpoint);

   protected:
    virtual void on_accept_connection() {}
    virtual void on_establish_connection() {}
    virtual void on_send_udp() {}
    virtual void on_receive_udp(const std::span<uint8_t> data, const asio::ip::udp::endpoint& endpoint) {}

    void start_read_tcp();
    void start_write_tcp();
    void start_write_tcp(const std::span<uint8_t> data);

   private:
    class connector {
     public:
      connector(connection* conn)
          : parent(conn) {}

      void listen_at(asio::ip::tcp::endpoint endpoint);
      void connect_to(asio::ip::tcp::endpoint endpoint);

      void send_udp(const std::span<uint8_t> data);
      void listen_udp(const asio::ip::udp::endpoint& endpoint);

      connection* parent = nullptr;
      scope<asio::ip::tcp::acceptor> tcp_acceptor;
      scope<asio::ip::tcp::socket> tcp_socket;
      scope<asio::ip::udp::socket> udp_socket;

      void on_accept_connection(const asio::error_code& ec, asio::ip::tcp::socket socket);
      void on_establish_connection(const asio::error_code& ec);
    };

    io& io_context;
    connector conn;

    constexpr static size_t kNetworkConnectionBufferSize = 8192;

    std::mutex io_mutex;
    async_buffer<kNetworkConnectionBufferSize> io_buffer;

    binding_point endpoint;

    connection_state_machine state_machine;

    void finish_read_tcp(const asio::error_code& ec, size_t bytes_transferred);
    void finish_read_udp(const asio::error_code& ec, size_t bytes_transferred);
    void finish_write_tcp(const asio::error_code& ec, size_t bytes_transferred);
    void finish_write_udp(const asio::error_code& ec, size_t bytes_transferred);
  };

}  // namespace other

#endif  // OTHER_NETWORK_NETWORK_CONNECTION_HPP
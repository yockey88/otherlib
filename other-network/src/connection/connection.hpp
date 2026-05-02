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

  class connection {
   public:
    connection(natural_t id, event_system& events, io& io_context, const binding_point& endpoint)
        : id(id), events(events), io_context(io_context), conn(this), endpoint(endpoint) {}
    virtual ~connection() = default;

    static scope<connection> tcp_connection(natural_t id, event_system& events, io& io_context, const binding_point& endpoint);
    static scope<connection> udp_connection(natural_t id, event_system& events, io& io_context, const binding_point& endpoint);

    void listen_on_tcp_endpoint(const binding_point& endpoint);
    void connect_to_tcp_endpoint(const binding_point& endpoint);

    void open_udp_endpoint(const binding_point& endpoint);

    void poll();

    virtual void on_accept_tcp_connection() {}
    virtual void on_establish_tcp_connection() {}

   protected:
    void start_read_tcp();
    void start_write_tcp();
    void start_write_tcp(const std::span<uint8_t> data);

    virtual void on_receive_tcp(const std::span<uint8_t> data) {}
    virtual void on_send_tcp(size_t bytes_transferred) {}
    virtual void on_receive_udp(const std::span<uint8_t> data, const asio::ip::udp::endpoint& endpoint) {}
    virtual void on_send_udp(size_t bytes_transferred) {}

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

    natural_t id;
    event_system& events;
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

#endif  // OTHER_NETWORK_CONNECTION_CONNECTION_HPP
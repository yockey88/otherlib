/**
 * \file network/tcp/connection.hpp
 **/
#ifndef OTHERLIB_NETWORK_TCP_CONNECTION_HPP
#define OTHERLIB_NETWORK_TCP_CONNECTION_HPP

#include "core/async_buffer.hpp"

#include "message/message.hpp"

namespace other {

  class tcp_transport_provider;

  class connection {
   public:
    connection(tcp_transport_provider* provider, natural_t id, const binding_point& endpoint, asio::ip::tcp::socket tcp_socket)
        : inactive(false), provider(provider), id(id), local_endpoint(endpoint) {
      this->tcp_socket = make_scope<asio::ip::tcp::socket>(std::move(tcp_socket));
    }
    virtual ~connection() = default;

    static scope<connection> create_tcp_connection(tcp_transport_provider* provider, natural_t id, const binding_point& endpoint, asio::ip::tcp::socket tcp_socket);

    inline void set_parent_connection_id(natural_t parent_id) { parent_connection_id = parent_id; }

    void poll();
    void reset();  // sort of shutdown, but keep if reconnect is requested
    void shutdown();

    void start_read();
    void write(const std::span<const uint8_t> data);

    asio::ip::tcp::endpoint get_remote_endpoint() const;
    asio::ip::tcp::endpoint get_local_endpoint() const;

   private:
    bool inactive = false;

    tcp_transport_provider* provider = nullptr;
    natural_t id;
    opt<natural_t> parent_connection_id;
    scope<asio::ip::tcp::socket> tcp_socket;

    constexpr static size_t kMaxReadBufferSize = 8192;
    async_buffer<kMaxReadBufferSize> buffer;

    binding_point local_endpoint;

    void on_connect(const asio::error_code& ec);

    void finish_read(const asio::error_code& ec, size_t bytes_transferred);
    void finish_write(const asio::error_code& ec, size_t bytes_transferred);
  };

}  // namespace other

#endif  // OTHERLIB_NETWORK_TCP_CONNECTION_HPP
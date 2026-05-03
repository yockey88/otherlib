/**
 * \file connection/connection.cpp
 **/
#include "connection/connection.hpp"

#include "network/network_thread.hpp"

#include "asio/asio/error.hpp"
#include "connection_state_maching.hpp"

namespace other {

  scope<connection> connection::create_tcp_connection(network_thread* thread, natural_t id, event_system& events, io& io_context, const binding_point& endpoint, asio::ip::tcp::socket tcp_socket) {
    return make_scope<connection>(thread, id, events, io_context, endpoint, std::move(tcp_socket));
  }

  scope<connection> connection::create_udp_connection(network_thread* thread, natural_t id, event_system& events, io& io_context, const binding_point& endpoint, asio::ip::udp::socket udp_socket) {
    return make_scope<connection>(thread, id, events, io_context, endpoint, std::move(udp_socket));
  }

  void connection::poll() {
    OTHER_ASSERT(parent_thread != nullptr, "Connection has null parent thread");

    if (buffer.has_pending_read_data()) {
      auto data = buffer.read();
      parent_thread->receive_data(id, data);
    }
  }

  void connection::shutdown() {
    OTHER_ASSERT(parent_thread != nullptr, "Connection has null parent thread");

    if (conn.tcp_socket) {
      conn.tcp_socket->shutdown(asio::ip::tcp::socket::shutdown_both);
    }

    if (conn.udp_socket) {
      conn.udp_socket->shutdown(asio::ip::udp::socket::shutdown_both);
    }
  }

  void connection::start_read() {
    if (buffer.is_reading()) {
      return;
    }

    buffer.start_read();
    if (conn.tcp_socket) {
      conn.tcp_socket->async_read_some(buffer.asio_read_buffer(), std::bind_front(&connection::finish_read, this));
    }
  }

  void connection::write(const std::vector<uint8_t>& data) {
  }

  void connection::finish_read(const asio::error_code& ec, size_t bytes_transferred) {
    if ((ec && ec == asio::error::operation_aborted) ||
        (ec && ec == asio::error::connection_reset) ||
        (ec && ec == asio::error::timed_out) ||
        (ec && ec == asio::error::eof)) {
      return;
    }

    if (ec) {
      CORE_LOG_ERROR("Error reading from TCP connection {}: {}", id, ec.message());
      return;
    }

    buffer.finish_read(bytes_transferred);
    buffer.start_read();
  }

  void connection::finish_write(const asio::error_code& ec, size_t bytes_transferred) {
  }

  // if ((ec && ec == asio::error::operation_aborted) ||
  //     (ec && ec == asio::error::connection_reset) ||
  //     (ec && ec == asio::error::timed_out) ||
  //     (ec && ec == asio::error::eof)) {
  //   return;
  // }

}  // namespace other
/**
 * \file connection/connection.cpp
 **/
#include "connection/connection.hpp"

#include "network/network_thread.hpp"

#include "asio/asio/error.hpp"

namespace other {

  scope<connection> connection::create_tcp_connection(network_thread* thread, natural_t id, io& io_context, const binding_point& endpoint, asio::ip::tcp::socket tcp_socket) {
    return make_scope<connection>(thread, id, io_context, endpoint, std::move(tcp_socket));
  }

  scope<connection> connection::create_udp_connection(network_thread* thread, natural_t id, io& io_context, const binding_point& endpoint, asio::ip::udp::socket udp_socket) {
    return make_scope<connection>(thread, id, io_context, endpoint, std::move(udp_socket));
  }

  void connection::poll() {
    OTHER_ASSERT(parent_thread != nullptr, "Connection has null parent thread");
    if (inactive) {
      return;
    }

    if (buffer.has_pending_read_data()) {
      auto data = buffer.read();
      parent_thread->receive_data(id, data);
    }
  }

  void connection::reset() {
    OTHER_ASSERT(parent_thread != nullptr, "Connection has null parent thread");

    conn.tcp_socket = nullptr;
    conn.udp_socket = nullptr;
    inactive = true;
  }

  void connection::shutdown() {
    OTHER_ASSERT(parent_thread != nullptr, "Connection has null parent thread");

    if (inactive) {
      return;
    }

    try {
      if (conn.tcp_socket) {
        conn.tcp_socket->shutdown(asio::ip::tcp::socket::shutdown_both);
      }

      if (conn.udp_socket) {
        conn.udp_socket->shutdown(asio::ip::udp::socket::shutdown_both);
      }
    } catch (const asio::system_error& e) {
      CORE_LOG_WARN("Connection closedown was not graceful: {}", e.what());
    } catch (const std::exception& e) {
      CORE_LOG_WARN("Connection closedown encountered an error: {}", e.what());
    } catch (...) {
      CORE_LOG_WARN("Connection closedown encountered an unknown error.");
      CORE_LOG_WARN("This may indicate that the connection was already closed or in an invalid state.");
    }
    inactive = true;
  }

  void connection::start_read() {
    if (buffer.is_reading() || inactive) {
      return;
    }

    CORE_LOG_TRACE("[CONNECTION {}: READ]", id);
    buffer.start_read();
    if (is_tcp()) {
      conn.tcp_socket->async_read_some(buffer.asio_read_buffer(), std::bind_front(&connection::finish_read, this));
    }
  }

  void connection::write(const std::span<const uint8_t> data) {
    if (inactive) {
      return;
    }

    buffer.buffer_write(data);
    if (buffer.is_writing()) {
      return;
    }

    buffer.start_write();
    if (is_tcp()) {
      CORE_LOG_TRACE("[CONNECTION {}: WRITE TCP]", id);
      conn.tcp_socket->async_write_some(buffer.asio_write_buffer(), std::bind_front(&connection::finish_write, this));
    } else if (is_udp()) {
      CORE_LOG_TRACE("[CONNECTION {}: WRITE UDP]", id);
      conn.udp_socket->async_send_to(buffer.asio_write_buffer(), remote_endpoint_udp(), std::bind_front(&connection::finish_write, this));
    }
  }

  asio::ip::tcp::endpoint connection::remote_endpoint() const {
    OTHER_ASSERT(conn.tcp_socket != nullptr, "Attempted to get remote endpoint of a connection without a TCP socket");
    return conn.tcp_socket->remote_endpoint();
  }

  asio::ip::udp::endpoint connection::remote_endpoint_udp() const {
    OTHER_ASSERT(conn.udp_socket != nullptr, "Attempted to get remote endpoint of a connection without a UDP socket");
    return conn.udp_socket->remote_endpoint();
  }

  asio::ip::tcp::endpoint connection::local_tcp_endpoint() const {
    OTHER_ASSERT(conn.tcp_socket != nullptr, "Attempted to get local endpoint of a connection without a TCP socket");
    return conn.tcp_socket->local_endpoint();
  }

  asio::ip::udp::endpoint connection::local_udp_endpoint() const {
    OTHER_ASSERT(conn.udp_socket != nullptr, "Attempted to get local endpoint of a connection without a UDP socket");
    return conn.udp_socket->local_endpoint();
  }

  void connection::finish_read(const asio::error_code& ec, size_t bytes_transferred) {
    if ((ec && ec == asio::error::operation_aborted) ||
        (ec && ec == asio::error::connection_reset) ||
        (ec && ec == asio::error::timed_out) ||
        (ec && ec == asio::error::eof)) {
      CORE_LOG_TRACE("[CONNECTION {}: CLOSED] Connection closed: {}", id, ec.message());
      inactive = true;
      parent_thread->notify_connection_closed(id);
      return;
    }
    if (inactive) {
      return;
    }

    if (ec) {
      CORE_LOG_ERROR("Error reading from TCP connection {}: {}", id, ec.message());
      return;
    }

    buffer.finish_read(bytes_transferred);
    start_read();
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
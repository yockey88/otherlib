/**
 * \file network/tcp/connection.cpp
 **/
#include "network/tcp/connection.hpp"

#include "network/tcp/tcp_transport_provider.hpp"

#include "asio/asio/error.hpp"

namespace other {

  scope<connection> connection::create_tcp_connection(tcp_transport_provider* provider, natural_t id, const binding_point& endpoint, asio::ip::tcp::socket tcp_socket) {
    return make_scope<connection>(provider, id, endpoint, std::move(tcp_socket));
  }

  void connection::reset() {
    OTHER_ASSERT(provider != nullptr, "Connection has null provider");

    tcp_socket = nullptr;
    inactive = true;
  }

  void connection::shutdown() {
    OTHER_ASSERT(provider != nullptr, "Connection has null provider");

    if (inactive || tcp_socket == nullptr) {
      return;
    }

    try {
      tcp_socket->shutdown(asio::ip::tcp::socket::shutdown_both);
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
    if (buffer.is_reading() || inactive || tcp_socket == nullptr) {
      return;
    }

    CORE_LOG_TRACE("[CONNECTION {}: START READ]", id);
    buffer.start_read();
    tcp_socket->async_read_some(buffer.asio_read_buffer(), std::bind_front(&connection::finish_read, this));
  }

  void connection::write(const std::span<const uint8_t> data) {
    if (inactive || tcp_socket == nullptr) {
      return;
    }

    buffer.buffer_write(data);
    if (buffer.is_writing()) {
      return;
    }

    buffer.start_write();
    CORE_LOG_TRACE("[CONNECTION {}: START WRITE TCP]", id);
    tcp_socket->async_write_some(buffer.asio_write_buffer(), std::bind_front(&connection::finish_write, this));
  }

  asio::ip::tcp::endpoint connection::get_remote_endpoint() const {
    OTHER_ASSERT(tcp_socket != nullptr, "Attempted to get remote endpoint of a connection without a TCP socket");
    return tcp_socket->remote_endpoint();
  }

  asio::ip::tcp::endpoint connection::get_local_endpoint() const {
    OTHER_ASSERT(tcp_socket != nullptr, "Attempted to get local endpoint of a connection without a TCP socket");
    return tcp_socket->local_endpoint();
  }

  void connection::on_connect(const asio::error_code& ec) {
    if ((ec && ec == asio::error::operation_aborted) ||
        (ec && ec == asio::error::connection_reset) ||
        (ec && ec == asio::error::timed_out) ||
        (ec && ec == asio::error::eof)) {
      CORE_LOG_TRACE("[CONNECTION {}: CONNECT FAILED] Connection failed to connect: {}", id, ec.message());
      inactive = true;
      provider->connection_socket_closed(id);
      return;
    }

    if (ec) {
      CORE_LOG_ERROR("Error connecting TCP connection {}: {}", id, ec.message());
      return;
    }

    CORE_LOG_TRACE("[CONNECTION {}: CONNECT SUCCESS]", id);
    start_read();
    provider->connection_accepted(id, local_endpoint);
  }

  void connection::finish_read(const asio::error_code& ec, size_t bytes_transferred) {
    if ((ec && ec == asio::error::operation_aborted) ||
        (ec && ec == asio::error::connection_reset) ||
        (ec && ec == asio::error::timed_out) ||
        (ec && ec == asio::error::eof)) {
      CORE_LOG_TRACE("[CONNECTION {}: CLOSED] Connection closed: {}", id, ec.message());
      inactive = true;
      provider->connection_socket_closed(id);
      return;
    }
    if (inactive) {
      return;
    }

    if (ec) {
      CORE_LOG_ERROR("Error reading from TCP connection {}: {}", id, ec.message());
      return;
    }

    CORE_LOG_TRACE("[CONNECTION {}: FINISH READ] Read {} bytes", id, bytes_transferred);
    buffer.finish_read(bytes_transferred);

    auto data = buffer.read();
    CORE_LOG_TRACE("[CONNECTION {}: RX DATA] Received {} bytes", id, data.size());
    provider->rx_data(id, data);

    start_read();
  }

  void connection::finish_write(const asio::error_code& ec, size_t bytes_transferred) {
    if ((ec && ec == asio::error::operation_aborted) ||
        (ec && ec == asio::error::connection_reset) ||
        (ec && ec == asio::error::timed_out) ||
        (ec && ec == asio::error::eof)) {
      return;
    }

    if (inactive) {
      return;
    }

    if (ec) {
      CORE_LOG_ERROR("Error writing to TCP connection {}: {}", id, ec.message());
      return;
    }

    CORE_LOG_TRACE("[CONNECTION {}: FINISH WRITE] Wrote {} bytes", id, bytes_transferred);
    buffer.finish_write();
    if (buffer.has_pending_write_data()) {
      auto data = buffer.pending_write_data();
      write(data);
    }
  }

}  // namespace other
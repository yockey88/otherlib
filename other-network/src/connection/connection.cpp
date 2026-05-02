/**
 * \file connection/connection.cpp
 **/
#include "connection/connection.hpp"

#include "asio/asio/error.hpp"
#include "connection_state_maching.hpp"

namespace other {

  scope<connection> connection::tcp_connection(natural_t id, event_system& events, io& io_context, const binding_point& endpoint) {
    return make_scope<connection>(id, events, io_context, endpoint);
  }

  scope<connection> connection::udp_connection(natural_t id, event_system& events, io& io_context, const binding_point& endpoint) {
    return make_scope<connection>(id, events, io_context, endpoint);
  }

  void connection::listen_on_tcp_endpoint(const binding_point& endpoint) {
    if (state_machine.get_current_state() != connection_state::DISCONNECTED) {
      CORE_LOG_WARN("Cannot start listening on endpoint {}:{} because connection is not in DISCONNECTED state.", endpoint.ip, endpoint.port);
      return;
    }

    state_machine.handle_event(connection_event::START_CONNECT);
    conn.listen_at(asio::ip::tcp::endpoint(asio::ip::address_v4(endpoint.ip), endpoint.port));
  }

  void connection::connect_to_tcp_endpoint(const binding_point& endpoint) {
    if (state_machine.get_current_state() != connection_state::DISCONNECTED) {
      CORE_LOG_WARN("Cannot start connecting to endpoint {}:{} because connection is not in DISCONNECTED state.", endpoint.ip, endpoint.port);
      return;
    }

    state_machine.handle_event(connection_event::START_CONNECT);
    conn.connect_to(asio::ip::tcp::endpoint(asio::ip::address_v4(endpoint.ip), endpoint.port));
  }

  void connection::open_udp_endpoint(const binding_point& endpoint) {
  }

  void connection::poll() {
    /// take from read queue
    std::vector<uint8_t> read_data;
    if (io_buffer.has_pending_read_data()) {
      std::lock_guard lock(io_mutex);
      read_data = io_buffer.read();
    }

    if (read_data.size() > 0) {
      on_receive_tcp(read_data);
    }
  }

  void connection::connector::listen_at(asio::ip::tcp::endpoint endpoint) {
    OTHER_ASSERT(parent != nullptr, "Connector has null parent connection.");
    OTHER_ASSERT(parent->state_machine.get_current_state() == connection_state::CONNECTING, "Cannot start listening at endpoint {}:{} because connection is not in CONNECTING state.", endpoint.address().to_string(), endpoint.port());
    tcp_acceptor = make_scope<asio::ip::tcp::acceptor>(parent->io_context.thread_pool, endpoint);
    tcp_acceptor->async_accept(std::bind_front(&connector::on_accept_connection, this));
  }

  void connection::connector::connect_to(asio::ip::tcp::endpoint endpoint) {
    OTHER_ASSERT(parent != nullptr, "Connector has null parent connection.");
    OTHER_ASSERT(parent->state_machine.get_current_state() == connection_state::CONNECTING, "Cannot start connecting to endpoint {}:{} because connection is not in CONNECTING state.", endpoint.address().to_string(), endpoint.port());
    tcp_socket = make_scope<asio::ip::tcp::socket>(parent->io_context.thread_pool);
    tcp_socket->async_connect(endpoint, std::bind_front(&connector::on_establish_connection, this));
  }

  void connection::connector::send_udp(const std::span<uint8_t> data) {
    OTHER_ASSERT(parent != nullptr, "Connector has null parent connection.");
    OTHER_ASSERT(parent->state_machine.get_current_state() == connection_state::CONNECTED, "Cannot send UDP data to endpoint {}:{} because connection is not in CONNECTED state.", parent->endpoint.ip, parent->endpoint.port);
    OTHER_ASSERT(udp_socket != nullptr && udp_socket->is_open(), "UDP socket is not open for connection to endpoint {}:{}", parent->endpoint.ip, parent->endpoint.port);
    // udp_socket->async_send(asio::buffer(data), 0, [this](const asio::error_code& ec, std::size_t bytes_sent) {
    //   if (ec) {
    //     CORE_LOG_ERROR("Error sending UDP data: {}", ec.message());
    //     parent->state_machine.handle_event(connection_event::SEND_FAILURE_NO_RETRY);
    //   } else {
    //     CORE_LOG_DEBUG("Sent {} bytes of UDP data", bytes_sent);
    //     parent->state_machine.handle_event(connection_event::SEND_SUCCESS);
    //     parent->on_send_udp();
    //   }
    // });
  }

  void connection::connector::listen_udp(const asio::ip::udp::endpoint& endpoint) {
    OTHER_ASSERT(parent != nullptr, "Connector has null parent connection.");
    OTHER_ASSERT(parent->state_machine.get_current_state() == connection_state::CONNECTED, "Cannot start listening for UDP data on endpoint {}:{} because connection is not in CONNECTED state.", parent->endpoint.ip, parent->endpoint.port);
    // udp_socket = make_scope<asio::ip::udp::socket>(parent->io_context.context, endpoint);
    // udp_socket->async_receive(endpoint, [this, endpoint](const asio::error_code& ec, std::size_t bytes_received) {
    //   if (ec) {
    //     CORE_LOG_ERROR("Error receiving UDP data: {}", ec.message());
    //   } else {
    //     CORE_LOG_DEBUG("Received {} bytes of UDP data", bytes_received);
    //     std::vector<uint8_t> data(bytes_received);
    //     udp_socket->receive(asio::buffer(data), 0);
    //     parent->on_receive_udp(data, endpoint);
    //   }
    // });
  }

  void connection::connector::on_accept_connection(const asio::error_code& ec, asio::ip::tcp::socket socket) {
    std::lock_guard lock(parent->io_mutex);
    if (ec) {
      CORE_LOG_ERROR("Error accepting connection: {}", ec.message());
      parent->state_machine.handle_event(connection_event::CONNECT_FAILURE_NO_RETRY);
      return;
    }

    CORE_LOG_DEBUG("Accepted new connection from {}", socket.remote_endpoint().address().to_string());
    parent->state_machine.handle_event(connection_event::CONNECT_SUCCESS);
    tcp_socket = make_scope<asio::ip::tcp::socket>(std::move(socket));
    parent->events.trigger_event("connection.tcp-accepted", parent->id);
  }

  void connection::connector::on_establish_connection(const asio::error_code& ec) {
    std::lock_guard lock(parent->io_mutex);
    if (ec) {
      CORE_LOG_ERROR("Error establishing connection: {}", ec.message());
      parent->state_machine.handle_event(connection_event::CONNECT_FAILURE_NO_RETRY);
      return;
    }

    CORE_LOG_DEBUG("Successfully connected to {}", tcp_socket->remote_endpoint().address().to_string());
    parent->state_machine.handle_event(connection_event::CONNECT_SUCCESS);
    parent->events.trigger_event("connection.tcp-established", parent->id);
  }

  void connection::start_read_tcp() {
    if (io_buffer.is_reading() || conn.tcp_socket == nullptr || !conn.tcp_socket->is_open()) {
      return;
    }

    io_buffer.start_read();
    conn.tcp_socket->async_receive(io_buffer.asio_read_buffer(), std::bind_front(&connection::finish_read_tcp, this));
  }

  void connection::start_write_tcp() {
    if (io_buffer.is_writing() || conn.tcp_socket == nullptr || !conn.tcp_socket->is_open()) {
      return;
    }
    if (!io_buffer.has_pending_write_data()) {
      return;
    }

    io_buffer.start_write();
    conn.tcp_socket->async_send(io_buffer.asio_write_buffer(), std::bind_front(&connection::finish_write_tcp, this));
  }

  void connection::start_write_tcp(const std::span<uint8_t> data) {
    if (io_buffer.is_writing()) {
      if (!(conn.tcp_socket == nullptr || !conn.tcp_socket->is_open())) {
        io_buffer.buffer_write(data);
      }
      return;
    }

    io_buffer.start_write();
    conn.tcp_socket->async_send(io_buffer.asio_write_buffer(), std::bind_front(&connection::finish_write_tcp, this));
  }

  void connection::finish_read_tcp(const asio::error_code& ec, size_t bytes_transferred) {
    std::lock_guard lock(io_mutex);
    if ((ec && ec == asio::error::operation_aborted) ||
        (ec && ec == asio::error::connection_reset) ||
        (ec && ec == asio::error::timed_out) ||
        (ec && ec == asio::error::eof)) {
      state_machine.handle_event(connection_event::CONNECTION_LOST_NO_RETRY);
      return;
    }

    if (ec) {
      CORE_LOG_ERROR("Error reading from TCP connection: {}", ec.message());
      state_machine.handle_event(connection_event::READ_FAILURE);
      return;
    }

    CORE_LOG_DEBUG("Received {} bytes of TCP data", bytes_transferred);
    state_machine.handle_event(connection_event::READ_SUCCESS);

    io_buffer.finish_read(bytes_transferred);
    start_read_tcp();
  }

  void connection::finish_read_udp(const asio::error_code& ec, size_t bytes_transferred) {
    // std::lock_guard lock(io_mutex);
  }

  void connection::finish_write_tcp(const asio::error_code& ec, size_t bytes_transferred) {
    std::lock_guard lock(io_mutex);
    if ((ec && ec == asio::error::operation_aborted) ||
        (ec && ec == asio::error::connection_reset) ||
        (ec && ec == asio::error::timed_out) ||
        (ec && ec == asio::error::eof)) {
      return;
    }

    if (ec) {
      CORE_LOG_ERROR("Error writing to TCP connection: {}", ec.message());
      state_machine.handle_event(connection_event::SEND_FAILURE_NO_RETRY);
      return;
    }

    state_machine.handle_event(connection_event::SEND_SUCCESS);

    io_buffer.finish_write();
    start_write_tcp();
  }

  void connection::finish_write_udp(const asio::error_code& ec, size_t bytes_transferred) {
  }

}  // namespace other
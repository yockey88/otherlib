/**
 * \file network/tcp/tcp_transport_provider.cpp
 **/
#include "network/tcp/tcp_transport_provider.hpp"

#include "core/profiler.hpp"

#include "network/asio_endpoint.hpp"
#include "network/network_error.hpp"
#include "network/network_thread.hpp"

#include "asio/asio/ip/address.hpp"
#include "message/message.hpp"
#include "message/messages.hpp"

namespace other {

  void tcp_transport_provider::on_initialize() {}

  void tcp_transport_provider::on_tick() {
    if (pending_destroy.empty()) {
      return;
    }
    /// retry retirement for connections whose aborted handlers had not drained yet
    std::deque<natural_t> still_pending;
    for (const natural_t connection_id : pending_destroy) {
      auto itr = active_connections.find(connection_id);
      if (itr == active_connections.end()) {
        continue;
      }
      if (itr->second->busy()) {
        still_pending.push_back(connection_id);
        continue;
      }
      active_connections.erase(itr);
    }
    pending_destroy = std::move(still_pending);
  }

  void tcp_transport_provider::on_begin_shutdown() {
    PROFILE_SECTION("tcp_transport_provider::on_begin_shutdown");
    /// snapshot ids: closing mutates the maps through the retirement path
    ostd::vector<natural_t> listeners;
    for (const auto& [id, acceptor] : active_tcp_listeners) {
      listeners.push_back(id);
    }
    for (const natural_t id : listeners) {
      close(id);
    }

    ostd::vector<natural_t> conns;
    for (const auto& [id, conn] : active_connections) {
      if (!conn->is_closing()) {
        conns.push_back(id);
      }
    }
    for (const natural_t id : conns) {
      auto itr = active_connections.find(id);
      if (itr != active_connections.end()) {
        itr->second->close(connection_close_reason::SHUTDOWN);
      }
    }
  }

  void tcp_transport_provider::on_shutdown() {
    /// aborted handlers drained through the shutdown pumps; anything left dies with us
    pending_destroy.clear();
    active_connections.clear();
    active_tcp_listeners.clear();
    connection_state_machines.clear();
  }

  void tcp_transport_provider::handle_state_event(natural_t connection_id, connection_event event) {
    auto itr = connection_state_machines.find(connection_id);
    if (itr == connection_state_machines.end()) {
      return;
    }
    itr->second.handle_event(event);
  }

  void tcp_transport_provider::on_start_listen(natural_t conn_id, const binding_point& endpoint) {
    PROFILE_SECTION("tcp_transport_provider::on_start_listen");
    OTHER_ASSERT(active_tcp_listeners.find(conn_id) == active_tcp_listeners.end(), "Listener ID {} is already in use", conn_id);

    scope<asio::ip::tcp::acceptor> acceptor;
    try {
      /// reuse_address stays OFF: on windows it would let a second bind silently
      ///  steal a taken port instead of failing
      acceptor = make_scope<asio::ip::tcp::acceptor>(net_io_ref().context, to_asio_endpoint<asio::ip::tcp>(endpoint), false);
    } catch (const asio::system_error& e) {
      /// bind/listen failure (port in use) is data for the caller, not a crash
      throw port_in_use_network_error(std::format("Failed to listen at {}:{}: {}", endpoint.ip, endpoint.port, e.what()));
    }

    auto [itr, success] = active_tcp_listeners.emplace(conn_id, std::move(acceptor));
    OTHER_ASSERT(success, "Failed to store TCP listener for endpoint {}:{}", endpoint.ip, endpoint.port);

    auto [state_itr, state_success] = connection_state_machines.emplace(conn_id, connection_state_machine());
    OTHER_ASSERT(state_success, "Failed to create connection state machine for listener with ID {}", conn_id);

    host_thread_ref().register_listener_route(conn_id, this, itr->second.get());

    state_itr->second.handle_event(connection_event::START_CONNECT);
    CORE_LOG_TRACE("[CONNECTION {}: LISTEN] {}", conn_id, endpoint);
    start_accept_on(conn_id, endpoint);
  }

  void tcp_transport_provider::start_accept_on(natural_t listener_id, const binding_point& endpoint) {
    auto itr = active_tcp_listeners.find(listener_id);
    if (itr == active_tcp_listeners.end() || !itr->second->is_open()) {
      return;
    }
    itr->second->async_accept(std::bind_front(&tcp_transport_provider::on_accepted, this, listener_id, endpoint));
  }

  void tcp_transport_provider::on_start_connect(natural_t conn_id, const binding_point& endpoint) {
    PROFILE_SECTION("tcp_transport_provider::on_start_connect");
    OTHER_ASSERT(active_connections.find(conn_id) == active_connections.end(), "Connection ID {} is already in use", conn_id);

    auto [state_itr, state_success] = connection_state_machines.emplace(conn_id, connection_state_machine());
    OTHER_ASSERT(state_success, "Failed to create connection state machine for connection with ID {}", conn_id);
    state_itr->second.handle_event(connection_event::START_CONNECT);

    auto [conn_itr, conn_success] = active_connections.emplace(
      conn_id, make_scope<connection>(this, conn_id, asio::ip::tcp::socket(net_io_ref().context)));
    OTHER_ASSERT(conn_success, "Failed to create connection for endpoint {}:{}", endpoint.ip, endpoint.port);

    /// route registered up front so tx/close during the dial resolve here; a failed
    ///  dial retires it through the closed path like any other teardown
    host_thread_ref().register_connection_route(conn_id, this, conn_itr->second.get());

    CORE_LOG_TRACE("[CONNECTION {}: CONNECT] {}", conn_id, endpoint);
    conn_itr->second->start_connect(to_asio_endpoint<asio::ip::tcp>(endpoint));
  }

  void tcp_transport_provider::tx_data(natural_t connection_id, ostd::vector<uint8_t>&& data) {
    auto itr = active_connections.find(connection_id);
    if (itr == active_connections.end()) {
      CORE_LOG_WARN("Attempted to send data on non-existent connection with ID {}", connection_id);
      return;
    }

    itr->second->send(std::move(data));
  }

  void tcp_transport_provider::close(natural_t connection_id) {
    PROFILE_SECTION("tcp_transport_provider::close");
    auto itr = active_connections.find(connection_id);
    if (itr != active_connections.end()) {
      itr->second->close(connection_close_reason::LOCAL_CLOSE);
      return;
    }

    auto acceptor_itr = active_tcp_listeners.find(connection_id);
    if (acceptor_itr == active_tcp_listeners.end()) {
      CORE_LOG_WARN("Attempted to close non-existent connection with ID {}", connection_id);
      return;
    }

    asio::error_code ec;
    acceptor_itr->second->close(ec);
    if (ec) {
      CORE_LOG_WARN("TCP listener closedown was not graceful: {}", ec.message());
    }
    handle_state_event(connection_id, connection_event::DISCONNECT_SUCCESS);
    connection_socket_closed(connection_id, connection_close_reason::LOCAL_CLOSE);
  }

  void tcp_transport_provider::connection_removed(natural_t connection_id) {
    connection_state_machines.erase(connection_id);
    active_tcp_listeners.erase(connection_id);
    destroy_if_idle(connection_id);
  }

  void tcp_transport_provider::destroy_if_idle(natural_t connection_id) {
    auto itr = active_connections.find(connection_id);
    if (itr == active_connections.end()) {
      return;
    }
    if (itr->second->busy()) {
      pending_destroy.push_back(connection_id);
      return;
    }
    active_connections.erase(itr);
  }

  void tcp_transport_provider::notify_conn_rx(natural_t connection_id, std::span<const uint8_t> data) {
    handle_state_event(connection_id, connection_event::READ_SUCCESS);
    rx_data(connection_id, data);
  }

  void tcp_transport_provider::notify_conn_closed(natural_t connection_id, connection_close_reason reason) {
    switch (reason) {
      case connection_close_reason::LOCAL_CLOSE:
      case connection_close_reason::SHUTDOWN:
        handle_state_event(connection_id, connection_event::START_DISCONNECT);
        break;
      case connection_close_reason::WRITE_ERROR:
      case connection_close_reason::BACKPRESSURE:
        handle_state_event(connection_id, connection_event::SEND_FAILURE_NO_RETRY);
        break;
      default:
        handle_state_event(connection_id, connection_event::READ_FAILURE);
        break;
    }
    handle_state_event(connection_id, connection_event::DISCONNECT_SUCCESS);
    connection_socket_closed(connection_id, reason);
  }

  void tcp_transport_provider::notify_connect_failed(natural_t connection_id) {
    handle_state_event(connection_id, connection_event::CONNECT_FAILURE_NO_RETRY);
    connection_socket_closed(connection_id, connection_close_reason::CONNECT_FAILED);
  }

  void tcp_transport_provider::notify_connect_succeeded(natural_t connection_id) {
    handle_state_event(connection_id, connection_event::CONNECT_SUCCESS);

    auto itr = active_connections.find(connection_id);
    if (itr == active_connections.end()) {
      return;
    }

    const asio::ip::tcp::endpoint remote = itr->second->get_remote_endpoint();
    itr->second->begin_read();
    connection_established(connection_id, binding_point::from_asio(remote.address(), remote.port()));
  }

  void tcp_transport_provider::on_accepted(natural_t listener_id, const binding_point& endpoint, asio::error_code ec, asio::ip::tcp::socket&& socket) {
    PROFILE_SECTION("tcp_transport_provider::on_accepted");
    if (ec) {
      if (ec == asio::error::operation_aborted) {
        /// acceptor closed; teardown already ran through close()
        return;
      }
      CORE_LOG_ERROR("Error accepting connection on {}:{}: {}", endpoint.ip, endpoint.port, ec.message());
      start_accept_on(listener_id, endpoint);
      return;
    }

    if (host_thread_ref().is_shutdown_pending()) {
      CORE_LOG_WARN("Received new connection while shutdown pending, rejecting");
      asio::error_code ignored;
      socket.close(ignored);
      start_accept_on(listener_id, endpoint);
      return;
    }

    const natural_t connection_id = host_thread_ref().generate_connection_id();
    auto [conn_itr, conn_success] = active_connections.emplace(connection_id, make_scope<connection>(this, connection_id, std::move(socket)));
    OTHER_ASSERT(conn_success, "Failed to store accepted connection {}", connection_id);

    auto [state_itr, state_success] = connection_state_machines.emplace(connection_id, connection_state_machine());
    OTHER_ASSERT(state_success, "Failed to create connection state machine for connection with ID {}", connection_id);
    state_itr->second.handle_event(connection_event::CONNECT_SUCCESS);

    host_thread_ref().register_connection_route(connection_id, this, conn_itr->second.get());

    const asio::ip::tcp::endpoint remote = conn_itr->second->get_remote_endpoint();
    CORE_LOG_TRACE("[CONNECTION {}: ACCEPTED] listener: {}", connection_id, listener_id);
    conn_itr->second->begin_read();
    connection_accepted(listener_id, connection_id, binding_point::from_asio(remote.address(), remote.port()));

    start_accept_on(listener_id, endpoint);
  }

}  // namespace other
/**
 * \file network/tcp/tcp_transport_provider.cpp
 **/
#include "network/tcp/tcp_transport_provider.hpp"

#include "network/network_thread.hpp"

#include "asio/asio/ip/address.hpp"
#include "message/message.hpp"
#include "message/messages.hpp"

namespace other {

  void tcp_transport_provider::on_initialize() {}

  void tcp_transport_provider::on_tick() {
    for (auto citr = active_connections.cbegin(); citr != active_connections.cend(); ++citr) {
      citr->second->poll();
    }
  }

  void tcp_transport_provider::on_begin_shutdown() {
    for (auto itr = active_tcp_listeners.begin(); itr != active_tcp_listeners.end(); ++itr) {
      auto state_itr = connection_state_machines.find(itr->first);
      if (state_itr != connection_state_machines.end()) {
        if (state_itr->second.get_current_state() == connection_state::CONNECTED ||
            state_itr->second.get_current_state() == connection_state::CONNECTING ||
            state_itr->second.get_current_state() == connection_state::RECONNECTING) {
          CORE_LOG_TRACE("[CONNECTION {}] Initiating shutdown of active listener", itr->first);
          itr->second->close();
        }
      }
    }

    for (auto itr = active_connections.begin(); itr != active_connections.end(); ++itr) {
      auto state_itr = connection_state_machines.find(itr->first);
      if (state_itr != connection_state_machines.end()) {
        if (state_itr->second.get_current_state() == connection_state::CONNECTED ||
            state_itr->second.get_current_state() == connection_state::CONNECTING ||
            state_itr->second.get_current_state() == connection_state::RECONNECTING) {
          CORE_LOG_TRACE("[CONNECTION {}] Initiating shutdown of active connection", itr->first);
          itr->second->shutdown();
        }
      }
    }
  }

  void tcp_transport_provider::on_shutdown() {
    OTHER_ASSERT(connection_state_machines.empty(), "Connection state machines should be empty on shutdown.");
    OTHER_ASSERT(active_connections.empty(), "Active connections should be empty on shutdown.");
    OTHER_ASSERT(active_tcp_listeners.empty(), "Active TCP listeners should be empty on shutdown.");
    OTHER_ASSERT(recently_closed_connections.empty(), "Recently closed connections should be empty on shutdown.");
  }

  void tcp_transport_provider::on_start_listen(natural_t conn_id, const binding_point& endpoint) {
    asio::ip::tcp::endpoint asio_endpoint(asio::ip::address_v4(endpoint.ip), endpoint.port);

    OTHER_ASSERT(active_tcp_listeners.find(conn_id) == active_tcp_listeners.end(), "Listener ID {} is already in use", conn_id);

    /// because we verified id was free before, this should never fail
    auto [itr, success] = active_tcp_listeners.emplace(conn_id, make_scope<asio::ip::tcp::acceptor>(net_io_ref().context, asio_endpoint));
    OTHER_ASSERT(success, "Failed to create TCP listener for endpoint {}:{}", endpoint.ip, endpoint.port);

    auto [state_itr, state_success] = connection_state_machines.emplace(conn_id, connection_state_machine());
    OTHER_ASSERT(state_success, "Failed to create connection state machine for listener with ID {}", conn_id);

    host_thread_ref().register_listener_route(conn_id, this, itr->second.get());

    state_itr->second.handle_event(connection_event::START_CONNECT);
    CORE_LOG_TRACE("[CONNECTION {}: LISTEN] {}", conn_id, endpoint);
    itr->second->async_accept(std::bind_front(&tcp_transport_provider::on_accepted, this, conn_id, endpoint));
  }

  void tcp_transport_provider::on_start_connect(natural_t conn_id, const binding_point& endpoint) {
    CORE_LOG_WARN("TCP connect unimplemented: attempted to connect to {}:{}", endpoint.ip, endpoint.port);
    // auto [state_itr, state_success] = connection_state_machines.emplace(id, connection_state_machine());
    // OTHER_ASSERT(state_success, "Failed to create connection state machine for connection with ID {}", id);

    // auto [itr, success] = active_connections.emplace(id, connection::create_tcp_connection(&host_thread_ref(), id, net_io_ref(), endpoint, asio::ip::tcp::socket(net_io_ref().context)));
    // if (!success) {
    //   CORE_LOG_ERROR("Failed to create connection for endpoint {}:{}", endpoint.ip, endpoint.port);
    //   connection_state_machines.erase(state_itr);
    //   return 0;
    // }

    // itr->second->start_connect();
    // CORE_LOG_TRACE("[CONNECTION {}: CONNECT] {}", id, endpoint);
  }

  void tcp_transport_provider::tx_data(natural_t connection_id, std::span<const uint8_t> data) {
    auto itr = active_connections.find(connection_id);
    if (itr == active_connections.end()) {
      CORE_LOG_WARN("Attempted to send data on non-existent connection with ID {}", connection_id);
      return;
    }

    itr->second->write(data);
  }

  void tcp_transport_provider::close(natural_t connection_id) {
    auto itr = active_connections.find(connection_id);
    if (itr == active_connections.end()) {
      auto acceptor_itr = active_tcp_listeners.find(connection_id);
      if (acceptor_itr != active_tcp_listeners.end()) {
        try {
          acceptor_itr->second->close();
        } catch (const asio::system_error& e) {
          CORE_LOG_WARN("TCP listener closedown was not graceful: {}", e.what());
        } catch (const std::exception& e) {
          CORE_LOG_WARN("TCP listener closedown encountered an error: {}", e.what());
        } catch (...) {
          CORE_LOG_WARN("TCP listener closedown encountered an unknown error.");
          CORE_LOG_WARN("This may indicate that the listener was already closed or in an invalid state.");
        }
      } else {
        CORE_LOG_WARN("Attempted to close non-existent connection with ID {}", connection_id);
      }
    } else {
      itr->second->shutdown();
    }
  }

  void tcp_transport_provider::connection_removed(natural_t connection_id) {
    active_connections.erase(connection_id);
    active_tcp_listeners.erase(connection_id);
    connection_state_machines.erase(connection_id);
  }

  void tcp_transport_provider::on_rx_data(natural_t connection_id, std::span<const uint8_t> data) {
    message msg(NOTIFICATION, RX_DATA);
    notification_rx_data notification_data{
      .connection_id = connection_id,
      .data = std::vector<uint8_t>(data.begin(), data.end()),
    };
    msg.data = serialize_direct(notification_data);
    host_thread_ref().send_to_driver(std::move(msg));
  }

  void tcp_transport_provider::on_connection_socket_closed(natural_t connection_id) {
    auto state_itr = connection_state_machines.find(connection_id);
    if (state_itr == connection_state_machines.end()) {
      CORE_LOG_ERROR("[CONNECTION {}] closed connection: no state machine found!", connection_id);
      return;
    }

    state_itr->second.handle_event(connection_event::DISCONNECT_SUCCESS);
  }

  void tcp_transport_provider::on_connection_socket_broken(natural_t connection_id) {
    CORE_LOG_TRACE("[CONNECTION {}] Broken", connection_id);
    auto state_itr = connection_state_machines.find(connection_id);
    if (state_itr == connection_state_machines.end()) {
      CORE_LOG_ERROR("[CONNECTION {}] broken connection: no state machine found!", connection_id);
      return;
    }
    state_itr->second.handle_event(connection_event::CONNECTION_LOST_NO_RETRY);
  }

  void tcp_transport_provider::on_accepted(natural_t id, const binding_point& endpoint, asio::error_code ec, asio::ip::tcp::socket&& socket) {
    if ((ec && ec == asio::error::operation_aborted) ||
        (ec && ec == asio::error::connection_reset) ||
        (ec && ec == asio::error::timed_out) ||
        (ec && ec == asio::error::eof)) {
      connection_socket_closed(id);
      return;
    }

    if (ec) {
      CORE_LOG_ERROR("Error accepting connection on {}: {}: {}", endpoint.ip, endpoint.port, ec.message());
      return;
    }

    register_new_connection(std::move(socket), endpoint, id);

    auto itr = active_tcp_listeners.find(id);
    OTHER_ASSERT(itr != active_tcp_listeners.end(), "Listener with ID {} not found when trying to listen for next connection", id);
    itr->second->async_accept(std::bind_front(&tcp_transport_provider::on_accepted, this, id, endpoint));
  }

  void tcp_transport_provider::register_new_connection(asio::ip::tcp::socket&& socket, const binding_point& endpoint, natural_t listener_conn_id) {
    OTHER_ASSERT(host_thread_ref().is_shutdown_pending() == false, "tcp_transport_provider has null host");

    if (host_thread_ref().is_shutdown_pending()) {
      CORE_LOG_WARN("Received new connection while shutdown pending, rejecting connection from {}", socket.remote_endpoint().address().to_string() + ":" + std::to_string(socket.remote_endpoint().port()));
      return;
    }

    natural_t connection_id = network_thread::generate_connection_id();
    binding_point local_bp = {};
    {
      auto [conn_itr, conn_success] = active_connections.emplace(connection_id, connection::create_tcp_connection(this, connection_id, endpoint, std::move(socket)));
      if (!conn_success) {
        CORE_LOG_ERROR("Failed to create connection for endpoint {}:{}", endpoint.ip, endpoint.port);
        return;
      }

      auto [state_itr, state_success] = connection_state_machines.emplace(connection_id, connection_state_machine());
      OTHER_ASSERT(state_success, "Failed to create connection state machine for connection with ID {}", connection_id);

      // skip straight to connected (mirrors prior accept behavior).
      state_itr->second.handle_event(connection_event::CONNECT_SUCCESS);
      CORE_LOG_TRACE("[CONNECTION {}: CONNECT] listener: {}", connection_id, listener_conn_id);

      host_thread_ref().register_connection_route(connection_id, this, conn_itr->second.get());

      asio::ip::tcp::endpoint local_endpoint = conn_itr->second->get_local_endpoint();
      local_bp = binding_point::from_asio(local_endpoint.address(), local_endpoint.port());
      conn_itr->second->start_read();
    }

    message msg(NOTIFICATION, CONNECT_CONNECTION);
    notification_connect_connection notification_data{
      .connection_endpoint = endpoint,
      .endpoint = local_bp,
      .connection_id = listener_conn_id,
      .new_connection_id = connection_id,
      .transport_hash = hash(),
    };
    msg.data = serialize_direct(notification_data);
    host_thread_ref().send_to_driver(std::move(msg));
  }

}  // namespace other
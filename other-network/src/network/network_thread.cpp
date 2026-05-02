/**
 * \file network/network_thread.cpp
 **/
#include "network/network_thread.hpp"

#include <asio/asio/ip/address_v4.hpp>

#include "core/defines.hpp"
#include "core/timer.hpp"
#include "thread/message.hpp"
#include "thread/messages.hpp"

#include "network/message.hpp"

namespace other {

  void network_thread::on_initialize() {
    bus.register_thread();
  }

  void network_thread::on_start() {
    message network_ready_msg;
    network_ready_msg.header = {
      .category = NOTIFICATION,
      .id = NETWORK_THREAD_READY,
    };
    bus.send_message(std::move(network_ready_msg));
  }

  void network_thread::on_shutdown() {
    message shutdown_msg;
    shutdown_msg.header = {
      .category = NOTIFICATION,
      .id = NETWORK_THREAD_SHUTDOWN_COMPLETE,
    };
    bus.send_message(std::move(shutdown_msg));
  }

  void network_thread::pump_thread() {
    network_io.context.poll();
    if (network_io.context.stopped()) {
      network_io.context.restart();
    }

    auto msg = bus.receive_message(duration_cast<milliseconds>(tick_duration(10)));
    if (msg.has_value()) {
      switch (msg->header.category) {
        case CONTROL:
          switch (msg->header.id) {
            case PING: handle_control_ping(std::move(*msg)); break;
            // case PONG: handle_control_pong(std::move(*msg)); break;
            default:
              CORE_LOG_WARN("Network thread received unknown CONTROL message ID {:#06x}", msg->header.id);
              break;
          }
          break;

        case COMMAND:
          switch (msg->header.id) {
            case SHUTDOWN_REQUEST: handle_command_shutdown_request(std::move(*msg)); break;
            case LISTEN_TCP_CONNECTION: handle_command_listen_tcp_connection(std::move(*msg)); break;
            case CONNECT_TCP_CONNECTION: handle_command_connect_tcp_connection(std::move(*msg)); break;
            case OPEN_UDP_CONNECTION: handle_command_open_udp_connection(std::move(*msg)); break;
            case CLOSE_CONNECTION: handle_command_close_connection(std::move(*msg)); break;
            default:
              CORE_LOG_WARN("Network thread received unknown COMMAND message ID {:#06x}", msg->header.id);
              break;
          }
          break;

        case REQUEST:
          switch (msg->header.id) {
            default:
              CORE_LOG_WARN("Network thread received unknown REQUEST message ID {:#06x}", msg->header.id);
              break;
          }
          break;

        default:
          CORE_LOG_WARN("Network thread received unknown message category {}", static_cast<int>(msg->header.category));
          break;
      }
    }

    if (current_state.shutdown_pending) {
      if (current_state.shutdown_complete) {
        return;
      }

      message msg;
      msg.header = {
        .category = ACKNOWLEDGEMENT,
        .id = ACK,
      };

      acknowledgement ackmsg;
      ackmsg.acked_header = {
        .category = COMMAND,
        .id = SHUTDOWN_REQUEST,
      };
      ackmsg.ack_nack = 1;
      msg.data.append_range(ackmsg.as_buffer());
      bus.send_message(std::move(msg));

      current_state.shutdown_complete = true;
    }
  }

  void network_thread::accept_connections(asio::ip::tcp::socket&& socket, const asio::error_code& ec) {
    if (ec && ec == asio::error::operation_aborted) {
      return;
    } else if (!ec && current_connections >= max_connections) {
      CORE_LOG_WARN("Maximum connections reached, rejecting new connection from {}", socket.remote_endpoint().address().to_string());
      socket.close();
      return;
    }
    CORE_LOG_DEBUG("Accepted new connection from {}", socket.remote_endpoint().address().to_string());
  }

  void network_thread::handle_control_ping(message&& msg) {
  }

  void network_thread::handle_command_shutdown_request(message&& msg) {
    CORE_LOG_DEBUG("Received shutdown request, shutting down network thread...");

    current_state.shutdown_pending = true;
  }

  /// \todo check for duplicate endpoints or other invalid connection parameters

  void network_thread::handle_command_listen_tcp_connection(message&& msg) {
    binding_point endpoint = *reinterpret_cast<const binding_point*>(msg.data.data());
    asio::ip::tcp::endpoint asio_endpoint(asio::ip::address_v4(endpoint.ip), endpoint.port);

    natural_t connection_id = generate_connection_id();
    auto [itr, success] = active_connections.emplace(connection_id, connection::tcp_connection(network_io, endpoint));
    if (!success) {
      CORE_LOG_ERROR("Failed to create connection for endpoint {}:{}", endpoint.ip, endpoint.port);
      return;
    }

    itr->second->listen_on_tcp_endpoint(endpoint);
  }

  void network_thread::handle_command_connect_tcp_connection(message&& msg) {
    binding_point endpoint = *reinterpret_cast<const binding_point*>(msg.data.data());
    asio::ip::tcp::endpoint asio_endpoint(asio::ip::address_v4(endpoint.ip), endpoint.port);

    natural_t connection_id = generate_connection_id();
    auto [itr, success] = active_connections.emplace(connection_id, connection::tcp_connection(network_io, endpoint));
    if (!success) {
      CORE_LOG_ERROR("Failed to create connection for endpoint {}:{}", endpoint.ip, endpoint.port);
      return;
    }

    itr->second->connect_to_tcp_endpoint(endpoint);
  }

  void network_thread::handle_command_open_udp_connection(message&& msg) {
    binding_point endpoint = *reinterpret_cast<const binding_point*>(msg.data.data());
    asio::ip::udp::endpoint asio_endpoint(asio::ip::address_v4(endpoint.ip), endpoint.port);

    natural_t connection_id = generate_connection_id();
    auto [itr, success] = active_connections.emplace(connection_id, connection::udp_connection(network_io, endpoint));
    if (!success) {
      CORE_LOG_ERROR("Failed to create connection for endpoint {}:{}", endpoint.ip, endpoint.port);
      return;
    }

    itr->second->open_udp_endpoint(endpoint);
  }

  void network_thread::handle_command_close_connection(message&& msg) {
    natural_t connection_id = *reinterpret_cast<const natural_t*>(msg.data.data());
    auto itr = active_connections.find(connection_id);
    if (itr == active_connections.end()) {
      CORE_LOG_WARN("Received request to close unknown connection ID {}", connection_id);
      return;
    }

    active_connections.erase(itr);
  }

}  // namespace other
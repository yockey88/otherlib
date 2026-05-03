/**
 * \file network/network_thread.cpp
 **/
#include "network/network_thread.hpp"

#include <cstdint>

#include <asio/asio/ip/address_v4.hpp>

#include "core/defines.hpp"
#include "core/time.hpp"
#include "thread/message.hpp"

#include "network/messages.hpp"
#include "network/network_error.hpp"

#include "message.hpp"

namespace other {

  void network_thread::receive_data(natural_t connection_id, const std::span<uint8_t> data) {
    std::lock_guard lock(current_state.mutex);
    pending_data.push({ connection_id, std::vector<uint8_t>(data.begin(), data.end()) });
  }

  void network_thread::notify_connection_closed(natural_t connection_id) {
    std::lock_guard lock(current_state.mutex);
    closed_connections.push(connection_id);
  }

  void network_thread::send_to_driver(message&& msg) {
    CORE_LOG_TRACE("[NETWORK THREAD TX: {}]", msg.header);
    bus.send_message(std::move(msg));
  }

  void network_thread::on_initialize() {
    bus.register_thread();
  }

  void network_thread::on_start() {
    message network_ready_msg(NOTIFICATION, NETWORK_THREAD_READY);
    send_to_driver(std::move(network_ready_msg));
  }

  void network_thread::on_shutdown() {
    message shutdown_msg(NOTIFICATION, NETWORK_THREAD_SHUTDOWN_COMPLETE);
    send_to_driver(std::move(shutdown_msg));
  }

  void network_thread::pump_thread() {
    network_io.context.poll();
    if (network_io.context.stopped()) {
      network_io.context.restart();
    }

    while (!closed_connections.empty()) {
      natural_t closed_id = closed_connections.front();
      closed_connections.pop();

      auto itr = active_connections.find(closed_id);
      OTHER_ASSERT(itr != active_connections.end(), "Received closed connection ID {} but no active connection found with that ID", closed_id);
      itr->second->reset();

      message msg(NOTIFICATION, CLOSE_TCP_CONNECTION);
      notification_close_tcp_connection notification_data{
        .connection_id = closed_id,
      };
      msg.data = serialize_direct(notification_data);
      send_to_driver(std::move(msg));
    }

    for (auto citr = active_connections.cbegin(); citr != active_connections.cend(); ++citr) {
      citr->second->poll();
    }

    while (!pending_data.empty()) {
      rx_packet packet = std::move(pending_data.front());
      pending_data.pop();

      message msg(NOTIFICATION, RX_DATA);
      notification_rx_data notification_data{
        .connection_id = packet.connection_id,
        .data = std::move(packet.data),
      };
      msg.data = serialize_direct(notification_data);
      send_to_driver(std::move(msg));
    }

    auto msg = bus.receive_message(microseconds(1));
    try {
      process_message(std::move(msg));
    } catch (const std::exception& e) {
      CORE_LOG_ERROR("Error processing message in network thread: {}", e.what());
    } catch (...) {
      CORE_LOG_ERROR("Unknown error processing message in network thread");
    }

    if (current_state.shutdown_pending) {
      if (current_state.shutdown_complete) {
        return;
      }

      active_connections.clear();
      active_tcp_listeners.clear();

      message_header shutdown_header(COMMAND, SHUTDOWN_REQUEST);
      natural_t ack_response_id = ack_list.get_pending_ack_response(shutdown_header);
      if (ack_response_id != 0) {
        message ack_msg(ACKNOWLEDGEMENT, ACK);
        acknowledgement_ack ack_data{
          .ack_id = ack_response_id,
          .acked_header = shutdown_header,
          .ack = 1,
        };
        ack_msg.data = serialize_direct(ack_data);
        send_to_driver(std::move(ack_msg));
      }

      CORE_LOG_DEBUG("Network thread shutdown complete");
      current_state.shutdown_complete = true;
    }
  }

  void network_thread::process_message(opt<message>&& msg) {
    if (msg.has_value()) {
      CORE_LOG_TRACE("[NETWORK THREAD RX: {}]", msg->header);
      switch (msg->header.category) {
        case CONTROL:
          switch (msg->header.id) {
            case PING: handle_control_ping(std::move(*msg)); break;
            default:
              throw std::runtime_error(std::format("Network thread received unknown CONTROL message ID {:#06x}", msg->header.id));
          }
          break;

        case COMMAND:
          switch (msg->header.id) {
            case SHUTDOWN_REQUEST: handle_command_shutdown_request(std::move(*msg)); break;
            case LISTEN_TCP_CONNECTION: handle_command_listen_tcp_connection(std::move(*msg)); break;
            case CONNECT_TCP_CONNECTION: handle_command_connect_tcp_connection(std::move(*msg)); break;
            case OPEN_UDP_CONNECTION: handle_command_open_udp_connection(std::move(*msg)); break;
            case CLOSE_TCP_CONNECTION: handle_command_close_tcp_connection(std::move(*msg)); break;
            default:
              throw std::runtime_error(std::format("Network thread received unknown COMMAND message ID {:#06x}", msg->header.id));
          }
          break;

        case REQUEST:
          switch (msg->header.id) {
            case ACK: handle_request_ack_process_msg(std::move(*msg)); break;
            default:
              throw std::runtime_error(std::format("Network thread received unknown REQUEST message ID {:#06x}", msg->header.id));
          }
          break;

        default:
          throw std::runtime_error(std::format("Network thread received message with unknown category {:#06x}", msg->header.category));
      }
    } else {
      /// no message received, just continue
    }
  }

  void network_thread::accept_tcp_connection(asio::ip::tcp::socket socket, const binding_point& endpoint, natural_t listener_conn_id) {
    std::lock_guard lock(current_state.mutex);

    natural_t connection_id = generate_connection_id();
    auto [itr, success] = active_connections.emplace(connection_id, connection::create_tcp_connection(this, connection_id, events, network_io, endpoint, std::move(socket)));
    if (!success) {
      CORE_LOG_ERROR("Failed to create connection for endpoint {}:{}", endpoint.ip, endpoint.port);
      return;
    }

    asio::ip::tcp::endpoint remote_endpoint = itr->second->remote_endpoint();
    asio::ip::tcp::endpoint local_endpoint = itr->second->local_tcp_endpoint();
    CORE_LOG_TRACE("[CONNECT] {} from {} ({}:{} to {}:{})", connection_id, listener_conn_id, remote_endpoint.address().to_string(), remote_endpoint.port(), local_endpoint.address().to_string(), local_endpoint.port());

    binding_point local_bp = binding_point::from_asio(local_endpoint.address(), local_endpoint.port());
    message msg(NOTIFICATION, CONNECT_TCP_CONNECTION);
    notification_connect_tcp_connection notification_data{
      .connection_endpoint = endpoint,
      .endpoint = local_bp,
      .connection_id = listener_conn_id,
      .new_connection_id = connection_id,
    };
    msg.data = serialize_direct(notification_data);
    send_to_driver(std::move(msg));

    itr->second->start_read();
  }

  void network_thread::finalize_connection_establishment(natural_t connection_id) {
    std::lock_guard lock(current_state.mutex);

    auto itr = active_connections.find(connection_id);
    if (itr == active_connections.end()) {
      CORE_LOG_ERROR("Failed to find connection with ID {} to finalize establishment", connection_id);
      return;
    }
    CORE_LOG_DEBUG("Finalized establishment of TCP connection with ID {}", connection_id);
  }

  void network_thread::handle_control_ping(message&& msg) {
  }

  void network_thread::handle_command_shutdown_request(message&& msg) {
    CORE_LOG_DEBUG("Received shutdown request, shutting down network thread...");
    current_state.shutdown_pending = true;
    for (auto& [id, listener] : active_tcp_listeners) {
      listener->close();
    }
    for (auto& [id, conn] : active_connections) {
      conn->shutdown();
    }
  }

  /// \todo check for duplicate endpoints or other invalid connection parameters

  void network_thread::handle_command_listen_tcp_connection(message&& msg) {
    command_listen_tcp_connection request = deserialize_direct<command_listen_tcp_connection>(msg.data).first;

    binding_point endpoint = request.endpoint;
    asio::ip::tcp::endpoint asio_endpoint(asio::ip::address_v4(endpoint.ip), endpoint.port);

    if (active_tcp_listeners.find(request.connection_id) != active_tcp_listeners.end()) {
      throw port_in_use_network_error(std::format("Listener with ID {} already exists for endpoint {}:{}", request.connection_id, endpoint.ip, endpoint.port));
    }

    auto [itr, success] = active_tcp_listeners.emplace(request.connection_id, make_scope<asio::ip::tcp::acceptor>(network_io.context, asio_endpoint));
    /// because we verified id was free before, this should never fail
    OTHER_ASSERT(success, "Failed to create TCP listener for endpoint {}:{}", endpoint.ip, endpoint.port);

    CORE_LOG_DEBUG("[LISTEN] {} @ {}", request.connection_id, endpoint);
    itr->second->async_accept([this, endpoint, listener_conn_id = request.connection_id](const asio::error_code& ec, asio::ip::tcp::socket socket) {
      if ((ec && ec == asio::error::operation_aborted) ||
          (ec && ec == asio::error::connection_reset) ||
          (ec && ec == asio::error::timed_out) ||
          (ec && ec == asio::error::eof)) {
        return;
      }

      if (ec) {
        CORE_LOG_ERROR("Error accepting connection: {}", ec.message());
      } else {
        accept_tcp_connection(std::move(socket), endpoint, listener_conn_id);
      }
    });
  }

  void network_thread::handle_command_connect_tcp_connection(message&& msg) {
    // notif
    // binding_point endpoint = *reinterpret_cast<const binding_point*>(msg.data.data());
    // asio::ip::tcp::endpoint asio_endpoint(asio::ip::address_v4(endpoint.ip), endpoint.port);

    // natural_t connection_id = generate_connection_id();
    // auto [itr, success] = active_connections.emplace(connection_id, connection::create_tcp_connection(this, connection_id, events, network_io, endpoint, asio::ip::tcp::socket(network_io.context)));
    // if (!success) {
    //   CORE_LOG_ERROR("Failed to create connection for endpoint {}:{}", endpoint.ip, endpoint.port);
    //   return;
    // }

    // itr->second->
  }

  void network_thread::handle_command_open_udp_connection(message&& msg) {
    // binding_point endpoint = *reinterpret_cast<const binding_point*>(msg.data.data());
    // asio::ip::udp::endpoint asio_endpoint(asio::ip::address_v4(endpoint.ip), endpoint.port);

    // natural_t connection_id = generate_connection_id();
    // auto [itr, success] = active_connections.emplace(connection_id, connection::udp_connection(connection_id, events, network_io, endpoint));
    // if (!success) {
    //   CORE_LOG_ERROR("Failed to create connection for endpoint {}:{}", endpoint.ip, endpoint.port);
    //   return;
    // }
  }

  void network_thread::handle_command_close_tcp_connection(message&& msg) {
    natural_t connection_id = *reinterpret_cast<const natural_t*>(msg.data.data());
    auto itr = active_connections.find(connection_id);
    if (itr == active_connections.end()) {
      CORE_LOG_WARN("Received request to close unknown connection ID {}", connection_id);
      return;
    }

    active_connections.erase(itr);
  }

  bool network_thread::immediately_acknowledge_message(const message_header& header) {
    if (header == message_header{ CONTROL, SHUTDOWN_REQUEST }) {
      return false;
    }
    return true;
  }

  void network_thread::handle_request_ack_process_msg(message&& msg) {
    OTHER_ASSERT(msg.data.size() >= sizeof(natural_t) + sizeof(message_header), "Invalid ACK message data size: {}", msg.data.size());
    natural_t ack_id = 0;
    message acked_msg;

    {
      request_acknowledgment request_data = deserialize_direct<request_acknowledgment>(msg.data).first;
      ack_id = request_data.ack_id;
      acked_msg.header = request_data.original_header;
      acked_msg.data = std::move(request_data.message_data);
    }

    uint8_t ack = 1;
    message_header original_header = acked_msg.header;
    try {
      process_message(std::move(acked_msg));
    } catch (const network_error& e) {
      CORE_LOG_ERROR("Error handler invoked: [{}]", original_header);
      CORE_LOG_ERROR("Failed to process message in network thread: {}", e.what());
      ack = 0;
    } catch (const std::runtime_error& e) {
      CORE_LOG_ERROR("Runtime error handling acknowledgment for message {}: {}", original_header, e.what());
      ack = 0;
    } catch (const std::exception& e) {
      CORE_LOG_ERROR("Error handling acknowledgment for message {}: {}", original_header, e.what());
      ack = 0;
    } catch (...) {
      CORE_LOG_ERROR("Unknown error handling acknowledgment for message {}", original_header);
      ack = 0;
    }

    if (immediately_acknowledge_message(acked_msg.header)) {
      message ack_msg;
      ack_msg.header = {
        .category = ACKNOWLEDGEMENT,
        .id = ACK,
      };
      acknowledgement_ack ack_data{
        .ack_id = ack_id,
        .acked_header = acked_msg.header,
        .ack = ack,
      };
      ack_msg.data = serialize_direct(ack_data);

      send_to_driver(std::move(ack_msg));
    } else {
      ack_list.add_pending_ack_response(ack_id, acked_msg.header);
    }
  }

}  // namespace other
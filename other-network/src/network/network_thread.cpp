/**
 * \file network/network_thread.cpp
 **/
#include "network/network_thread.hpp"

#include <cstdint>

#include <asio/asio/ip/address_v4.hpp>

#include "core/defines.hpp"
#include "core/time.hpp"
#include "core/timer.hpp"
#include "thread/message.hpp"
#include "thread/messages.hpp"

#include "network/message.hpp"

namespace other {

  void network_thread::receive_data(natural_t connection_id, const std::span<uint8_t> data) {
  }

  void network_thread::send_to_driver(message&& msg) {
    CORE_LOG_TRACE("[NETWORK THREAD TX: {}]", msg.header);
    bus.send_message(std::move(msg));
  }

  void network_thread::on_initialize() {
    bus.register_thread();

    // events.register_event("connection.tcp-accepted");
    // events.add_listener("connection.tcp-accepted", [this](const value& data) {
    //   OTHER_ASSERT(data.type() == value_type::UINT64, "Expected uint64 data for connection.tcp-accepted event, got {}", data.type());
    // });
    // events.register_event("connection.tcp-established");
    // events.add_listener("connection.tcp-established", [this](const value& data) {
    //   OTHER_ASSERT(data.type() == value_type::UINT64, "Expected uint64 data for connection.tcp-established event, got {}", data.type());
    // });
  }

  void network_thread::on_start() {
    message network_ready_msg;
    network_ready_msg.header = {
      .category = NOTIFICATION,
      .id = NETWORK_THREAD_READY,
    };
    send_to_driver(std::move(network_ready_msg));
  }

  void network_thread::on_shutdown() {
    message shutdown_msg;
    shutdown_msg.header = {
      .category = NOTIFICATION,
      .id = NETWORK_THREAD_SHUTDOWN_COMPLETE,
    };
    send_to_driver(std::move(shutdown_msg));
  }

  void network_thread::pump_thread() {
    network_io.context.poll();
    if (network_io.context.stopped()) {
      network_io.context.restart();
    }

    for (auto& [id, conn] : active_connections) {
      conn->poll();
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

      for (auto& [id, conn] : active_connections) {
        conn->shutdown();
      }

      message_header shutdown_header(COMMAND, SHUTDOWN_REQUEST);
      natural_t ack_response_id = ack_list.get_pending_ack_response(shutdown_header);
      if (ack_response_id != 0) {
        message ack_msg;
        ack_msg.header = {
          .category = ACKNOWLEDGEMENT,
          .id = ACK,
        };
        const uint8_t* ack_id_data = reinterpret_cast<const uint8_t*>(&ack_response_id);
        const uint8_t* original_header_data = reinterpret_cast<const uint8_t*>(&shutdown_header);
        ack_msg.data.append_range(std::span(ack_id_data, sizeof(natural_t)));
        ack_msg.data.append_range(std::span(original_header_data, sizeof(message_header)));
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
            case CLOSE_CONNECTION: handle_command_close_connection(std::move(*msg)); break;
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

  void network_thread::accept_tcp_connection(asio::ip::tcp::socket socket, const binding_point& endpoint) {
    std::lock_guard lock(current_state.mutex);

    natural_t connection_id = generate_connection_id();
    auto [itr, success] = active_connections.emplace(connection_id, connection::create_tcp_connection(this, connection_id, events, network_io, endpoint, std::move(socket)));
    if (!success) {
      CORE_LOG_ERROR("Failed to create connection for endpoint {}:{}", endpoint.ip, endpoint.port);
      return;
    }

    CORE_LOG_DEBUG("Accepted new TCP connection with ID {} from {}:{}", connection_id, endpoint.ip, endpoint.port);
    message msg;
    msg.header = {
      .category = NOTIFICATION,
      .id = NEW_TCP_CONNECTION_ACCEPTED,
    };
    const uint8_t* id = reinterpret_cast<const uint8_t*>(&connection_id);
    msg.data.append_range(std::span(id, sizeof(natural_t)));
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
  }

  /// \todo check for duplicate endpoints or other invalid connection parameters

  void network_thread::handle_command_listen_tcp_connection(message&& msg) {
    binding_point endpoint = *reinterpret_cast<const binding_point*>(msg.data.data());
    asio::ip::tcp::endpoint asio_endpoint(asio::ip::address_v4(endpoint.ip), endpoint.port);

    natural_t listener_id = generate_listener_id();
    auto [itr, success] = active_tcp_listeners.emplace(listener_id, make_scope<asio::ip::tcp::acceptor>(network_io.thread_pool, asio_endpoint));
    if (!success) {
      CORE_LOG_ERROR("Failed to create TCP acceptor for endpoint {}:{}", endpoint.ip, endpoint.port);
      return;
    }

    itr->second->async_accept([this, endpoint](const asio::error_code& ec, asio::ip::tcp::socket socket) {
      if ((ec && ec == asio::error::operation_aborted) ||
          (ec && ec == asio::error::connection_reset) ||
          (ec && ec == asio::error::timed_out) ||
          (ec && ec == asio::error::eof)) {
        return;
      }

      if (ec) {
        CORE_LOG_ERROR("Error accepting connection: {}", ec.message());
      } else {
        accept_tcp_connection(std::move(socket), endpoint);
      }
    });
  }

  void network_thread::handle_command_connect_tcp_connection(message&& msg) {
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

  void network_thread::handle_command_close_connection(message&& msg) {
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

    natural_t ack_id = *reinterpret_cast<const natural_t*>(msg.data.data());
    message acked_msg;
    {
      message_header acked_header = *reinterpret_cast<const message_header*>(msg.data.data() + sizeof(natural_t));
      std::span<const uint8_t> original_data(msg.data.data() + sizeof(natural_t) + sizeof(message_header), msg.data.size() - sizeof(natural_t) - sizeof(message_header));

      acked_msg.header = acked_header;
      acked_msg.data.append_range(original_data);
    }

    uint8_t ack = 1;
    try {
      process_message(std::move(acked_msg));
    } catch (const std::exception& e) {
      CORE_LOG_ERROR("Error handling acknowledgment for message {}: {}", acked_msg.header, e.what());
      ack = 0;
    } catch (...) {
      CORE_LOG_ERROR("Unknown error handling acknowledgment for message {}", acked_msg.header);
      ack = 0;
    }

    if (immediately_acknowledge_message(acked_msg.header)) {
      message ack_msg;
      ack_msg.header = {
        .category = ACKNOWLEDGEMENT,
        .id = ACK,
      };
      const uint8_t* ack_id_data = reinterpret_cast<const uint8_t*>(&ack_id);
      const uint8_t* header_data = reinterpret_cast<const uint8_t*>(&acked_msg.header);
      ack_msg.data.append_range(std::span(ack_id_data, sizeof(natural_t)));
      ack_msg.data.append_range(std::span(header_data, sizeof(message_header)));
      ack_msg.data.push_back(ack);

      send_to_driver(std::move(ack_msg));
    } else {
      ack_list.add_pending_ack_response(ack_id, acked_msg.header);
    }
  }

}  // namespace other
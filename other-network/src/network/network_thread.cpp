/**
 * \file network/network_thread.cpp
 **/
#include "network/network_thread.hpp"

#include "core/defines.hpp"
#include "thread/message.hpp"

#include "asio/asio/ip/address_v4.hpp"

namespace other {

  void network_thread::report_connection_closed(natural_t session_id) {
    auto itr = client_endpoints.find(session_id);
    if (itr != client_endpoints.end()) {
      CORE_LOG_DEBUG("Connection closed for client {}", session_id);
      itr->second.active_session->finalize();

      {
        message msg;
        msg.header = {
          .category = NOTIFICATION,
          .id = SESSION_CLOSED,
        };

        const uint8_t* session_id_bytes = reinterpret_cast<const uint8_t*>(&session_id);
        msg.data.append_range(std::span(session_id_bytes, sizeof(natural_t)));
        bus.send_message(std::move(msg));
      }

      client_endpoints.erase(itr);
      --current_connections;
    }
  }

  void network_thread::report_connection_error(session* cli, const asio::error_code& ec) {
    OTHER_ASSERT(cli != nullptr, "Client pointer is null");
    CORE_LOG_ERROR("Connection error for client {}: {}", cli->session_id, ec.message());
    report_connection_closed(cli->session_id);
  }

  void network_thread::report_connection_check_in(natural_t connection_id, integer_t session_id) {
    auto itr = std::ranges::find_if(pending_connections, [&](const connection& conn) { return conn.session_id == connection_id; });
    if (itr == pending_connections.end()) {
      CORE_LOG_ERROR("Failed to find connection id to report check in : {}", connection_id);
      return;
    }

    CORE_LOG_DEBUG("Connection {} checking in with session id {}", connection_id, session_id);
    {
      auto cb_itr = check_in_listeners.find(session_id);
      if (cb_itr == check_in_listeners.end()) {
        CORE_LOG_ERROR("Can not accept check in from session with incorrect session id {} (connection {})", session_id, connection_id);
        return;
      }
      cb_itr->second(session_id);
      check_in_listeners.erase(cb_itr);
    }

    auto [conn_itr, success] = client_endpoints.insert({ connection_id, std::move(*itr) });
    if (conn_itr == client_endpoints.end()) {
      CORE_LOG_ERROR("Failed to fully accept session check in : {} (connection {})", session_id, connection_id);
      return;
    }
    pending_connections.erase(itr);

    CORE_LOG_INFO("Session {} successfully checked in", session_id);
  }

  void network_thread::on_initialize() {
    bus.register_thread();
  }

  void network_thread::on_start() {
  }

  void network_thread::on_shutdown() {
    client_endpoints.clear();
    net_context = nullptr;
    CORE_LOG_DEBUG("Network thread shutdown complete.");
  }

  void network_thread::pump_thread() {
    net_context->io_context.poll();
    if (net_context->io_context.stopped()) {
      net_context->io_context.restart();
    }

    auto msg = bus.receive_message();
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
            case SESSION_LISTEN_FOR: handle_command_session_listen_for(std::move(*msg)); break;
            case SESSION_CHECK_IN: handle_command_session_check_in(std::move(*msg)); break;
            default:
              CORE_LOG_WARN("Network thread received unknown COMMAND message ID {:#06x}", msg->header.id);
              break;
          }
          break;

        case REQUEST:
          switch (msg->header.id) {
            case SESSION_CHECK_IN: handle_request_session_check_in(std::move(*msg)); break;
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

    for (auto& conn : pending_connections) {
      try {
        conn.active_session->poll();
      } catch (const network_packet_parse_error& e) {
        CORE_LOG_ERROR("Error parsing network packet from session {}: {}", conn.session_id, e.what());
      }
    }

    for (auto& [id, conn] : client_endpoints) {
      try {
        conn.active_session->poll();
      } catch (const network_packet_parse_error& e) {
        CORE_LOG_ERROR("Error parsing network packet from session {}: {}", id, e.what());
      }
    }

    if (current_state.shutdown_pending && client_endpoints.size() == 0) {
      if (current_state.shutdown_complete) {
        return;
      }
      CORE_LOG_DEBUG("All connections closed, completing network thread shutdown.");

      message msg;
      msg.header = {
        .category = ACKNOWLEDGEMENT,
        .id = ACK,
      };
      message_header acked_header = {
        .category = COMMAND,
        .id = SHUTDOWN_REQUEST,
      };
      const uint8_t* acked_header_bytes = reinterpret_cast<const uint8_t*>(&acked_header);
      msg.data.append_range(std::span(acked_header_bytes, sizeof(message_header)));
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

    if (!ec) {
      natural_t conn_id = get_next_connection_id();
      auto itr = pending_connections.insert(pending_connections.end(), connection{
                                                                         .session_id = (integer_t)conn_id,
                                                                         .endpoint = binding_point{ socket.remote_endpoint().address().to_v4().to_uint(), static_cast<uint16_t>(socket.remote_endpoint().port()) },
                                                                         .active_session = make_scope<session>(this, conn_id, net_context->io_context, std::move(socket)),
                                                                       });
      if (itr == pending_connections.end()) {
        CORE_LOG_ERROR("Failed to add new connection to client endpoints");
        return;
      }
      ++current_connections;
      itr->active_session->start_initialization();

      // Continue accepting new connections
      net_context->acceptor.async_accept([this](asio::error_code ec, asio::ip::tcp::socket socket) {
        accept_connections(std::move(socket), ec);
      });
    } else {
      CORE_LOG_ERROR("Error accepting connection: {}", ec.message());
    }
  }

  void network_thread::open_session_and_check_in_at(integer_t session_id, uint16_t port) {
    {
      auto itr = client_endpoints.find(session_id);
      if (itr != client_endpoints.end()) {
        CORE_LOG_WARN("Session ID {} is already connected", session_id);
        return;
      }
    }

    auto [itr, success] = client_endpoints.emplace(session_id, connection{
                                                                 .session_id = session_id,
                                                                 .endpoint = binding_point{ 0, port },
                                                                 .active_session = make_scope<session>(this, session_id, net_context->io_context),
                                                               });
    if (!success) {
      CORE_LOG_ERROR("Failed to add new connection to client endpoints");
      return;
    }

    asio::ip::tcp::endpoint ep(asio::ip::make_address_v4("127.0.0.1"), port);
    itr->second.active_session->socket.async_connect(ep, [this, session_id](const asio::error_code& ec) {
      if (!ec) {
        CORE_LOG_INFO("Successfully connected to other application with session ID {}", session_id);
        auto conn_itr = client_endpoints.find(session_id);
        OTHER_ASSERT(conn_itr != client_endpoints.end(), "Connection not found for session ID {}", session_id);
        conn_itr->second.active_session->check_in();
      } else {
        CORE_LOG_ERROR("Failed to connect to other application with session ID {}: {}", session_id, ec.message());
        client_endpoints.erase(session_id);
      }
    });
    CORE_LOG_TRACE("Attempting to connect to other application at port {} for session ID {}", port, session_id);
  }

  void network_thread::handle_control_ping(message&& msg) {
    // Respond with PONG
    message pong_msg;
    pong_msg.header = {
      .category = CONTROL,
      .id = PONG,
    };

    integer_t session_id = 0;  // network thread uses session id 0
    const uint8_t* session_id_bytes = reinterpret_cast<const uint8_t*>(&session_id);
    pong_msg.data.append_range(std::span(session_id_bytes, sizeof(integer_t)));

    bus.send_message(std::move(pong_msg));
  }

  void network_thread::handle_command_shutdown_request(message&& msg) {
    CORE_LOG_DEBUG("Received shutdown request, shutting down network thread...");

    try {
      net_context->acceptor.close();
    } catch (...) {
    }

    for (auto& [id, conn] : client_endpoints) {
      conn.active_session->shutdown();
    }
    current_state.shutdown_pending = true;
  }

  void network_thread::handle_command_session_check_in(message&& msg) {
    OTHER_ASSERT(msg.data.size() >= sizeof(integer_t) + sizeof(uint16_t), "Invalid session check-in message size");
    integer_t session_id = *reinterpret_cast<const integer_t*>(msg.data.data());
    uint16_t port = *reinterpret_cast<const uint16_t*>(msg.data.data() + sizeof(integer_t));

    CORE_LOG_DEBUG("CHECK-IN [{} @ {}]", session_id, port);
    open_session_and_check_in_at(session_id, port);
  }

  void network_thread::handle_command_session_listen_for(message&& msg) {
    OTHER_ASSERT(msg.data.size() >= sizeof(uint16_t), "Invalid session listen for message size");
    uint16_t port = *reinterpret_cast<const uint16_t*>(msg.data.data());

    net_context->acceptor = asio::ip::tcp::acceptor(net_context->io_context, asio::ip::tcp::endpoint(asio::ip::address_v4::any(), port));
    CORE_LOG_INFO("Network thread listening for incoming connections on port {}", port);
    net_context->acceptor.async_accept([this](asio::error_code ec, asio::ip::tcp::socket socket) {
      accept_connections(std::move(socket), ec);
    });

    message ack_msg;
    ack_msg.header = {
      .category = ACKNOWLEDGEMENT,
      .id = ACK,
    };
    message_header acked_header = {
      .category = COMMAND,
      .id = SESSION_LISTEN_FOR,
    };
    const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&acked_header);
    ack_msg.data.append_range(std::span(bytes, sizeof(message_header)));
    bus.send_message(std::move(ack_msg));
  }

  void network_thread::handle_request_session_check_in(message&& msg) {
    OTHER_ASSERT(msg.data.size() >= sizeof(integer_t), "Invalid session check-in message size");
    integer_t session_id = *reinterpret_cast<const integer_t*>(msg.data.data());
    CORE_LOG_DEBUG("Preparing event listener for CHECK-IN [{}]", session_id);

    auto callback = [&](integer_t session_id) {
      message msg;
      msg.header = {
        .category = RESPONSE,
        .id = SESSION_CHECK_IN,
      };
      const uint8_t* id_bytes = reinterpret_cast<const uint8_t*>(&session_id);
      msg.data.append_range(std::span(id_bytes, sizeof(integer_t)));
      bus.send_message(std::move(msg));
    };

    auto [itr, inserted] = check_in_listeners.insert({ session_id, callback });
    CORE_LOG_DEBUG(" - Registered check-in listener for session ID {}", session_id);
  }

}  // namespace other
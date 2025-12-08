/**
 * \file network/network_thread.cpp
 **/
#include "network/network_thread.hpp"

#include "core/defines.hpp"
#include "thread/message.hpp"
#include "thread/messages.hpp"

#include "asio/asio/ip/address_v4.hpp"

namespace other {

  void network_thread::report_connection_closed(natural_t connection_id, integer_t session_id) {
    /// \todo: don't erase the connections because they may reconnect, just mark them as closed
    ///           we need to add a mechanism to know when to fully remove them
    auto itr = client_endpoints.find(connection_id);
    if (itr != client_endpoints.end()) {
      CORE_LOG_DEBUG("Closing connection [{}] session {}", connection_id, session_id);
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
    report_connection_closed(cli->connection_id, cli->session_id);
  }

  void network_thread::report_connection_check_in(natural_t connection_id, integer_t session_id) {
    auto itr = std::ranges::find_if(pending_connections, [&](const connection& conn) { return conn.connection_number == connection_id; });
    if (itr == pending_connections.end()) {
      CORE_LOG_ERROR("Failed to find connection id to report check in : {}", connection_id);
      return;
    }
    /// this override works because either we opened the session and had it from the start, or the remote session
    ///   set it and this is correct
    itr->session_id = session_id;

    /// callback if any
    auto cb_itr = check_in_listeners.find(session_id);
    if (cb_itr != check_in_listeners.end()) {
      cb_itr->second(session_id);
      check_in_listeners.erase(cb_itr);
    }
    /// otherwise we have to report a new connection to the server
    else {
      message notif_msg;
      notif_msg.header = {
        .category = NOTIFICATION,
        .id = SESSION_CHECK_IN,
      };
      const uint8_t* session_id_bytes = reinterpret_cast<const uint8_t*>(&session_id);
      notif_msg.data.append_range(std::span(session_id_bytes, sizeof(integer_t)));
      bus.send_message(std::move(notif_msg));
    }

    auto [conn_itr, success] = client_endpoints.insert({ connection_id, std::move(*itr) });
    if (conn_itr == client_endpoints.end()) {
      CORE_LOG_ERROR("Failed to fully accept session check in : {} (connection {})", session_id, connection_id);
      return;
    }
    pending_connections.erase(itr);

    CORE_LOG_INFO("Session [{}] successfully checked in", session_id);
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
            case SESSION_CONNECT_TO: handle_command_session_connect_to(std::move(*msg)); break;
            case SESSION_CHECK_IN: handle_command_session_check_in(std::move(*msg)); break;
            case SESSION_TX_MESSAGE: handle_command_session_tx_message(std::move(*msg)); break;
            case ENVIRONMENT_LOAD_SCENE: handle_command_environment_load_scene(std::move(*msg)); break;
            default:
              CORE_LOG_WARN("Network thread received unknown COMMAND message ID {:#06x}", msg->header.id);
              break;
          }
          break;

        case REQUEST:
          switch (msg->header.id) {
            case SESSION_CHECK_IN: handle_request_session_check_in(std::move(*msg)); break;
            case NEW_UDP_STREAM_BINDING: handle_request_new_udp_stream_binding(std::move(*msg)); break;
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
      msg.data.push_back(0x01);
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

    if (!ec) {
      natural_t conn_id = get_next_session_id();
      auto itr = pending_connections.insert(pending_connections.end(), connection{
                                                                         .connection_number = current_connections,
                                                                         .session_id = (integer_t)conn_id,
                                                                         .endpoint = binding_point{ socket.remote_endpoint().address().to_v4().to_uint(), static_cast<uint16_t>(socket.remote_endpoint().port()) },
                                                                         .active_session = make_scope<session>(this, current_connections, conn_id, net_context->io_context, std::move(socket)),
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

    natural_t connection_number = current_connections;
    auto [itr, success] = client_endpoints.emplace(connection_number, connection{
                                                                        .connection_number = connection_number,
                                                                        .session_id = session_id,
                                                                        .endpoint = binding_point{ 0, port },
                                                                        .active_session = make_scope<session>(this, connection_number, session_id, net_context->io_context),
                                                                      });
    if (!success) {
      CORE_LOG_ERROR("Failed to add new connection to client endpoints");
      return;
    }
    ++current_connections;

    asio::ip::tcp::endpoint ep(asio::ip::make_address_v4("127.0.0.1"), port);
    itr->second.active_session->socket.async_connect(ep, [this, connection_number, session_id](const asio::error_code& ec) {
      if (!ec) {
        CORE_LOG_INFO("Successfully connected to other application with session ID {} (conn: {})", session_id, connection_number);
        auto conn_itr = client_endpoints.find(connection_number);
        OTHER_ASSERT(conn_itr != client_endpoints.end(), "Connection not found for session ID {} (conn: {})", session_id, connection_number);
        conn_itr->second.active_session->check_in();
      } else {
        CORE_LOG_ERROR("Failed to connect to other application with session ID {}: {}", session_id, ec.message());
        client_endpoints.erase(connection_number);
      }
    });
    CORE_LOG_TRACE("Attempting to connect to other application at port {} for session ID {} (conn: {})", port, session_id, connection_number);
  }

  void network_thread::open_session_and_connect_to(const binding_point& bp) {
    CORE_LOG_INFO("Opening session and connecting to {}", binding_point::write_string(bp));
    {
      auto itr = std::ranges::find_if(client_endpoints, [&](const auto& pair) { return pair.second.endpoint.ip == bp.ip && pair.second.endpoint.port == bp.port; });
      if (itr != client_endpoints.end()) {
        CORE_LOG_WARN("Already connected to endpoint {}", binding_point::write_string(bp));
        return;
      }
    }

    auto itr = pending_connections.insert(pending_connections.end(), connection{
                                                                       .connection_number = current_connections,
                                                                       .session_id = session::kInvalidSessionId,
                                                                       .endpoint = bp,
                                                                       .active_session = make_scope<session>(this, current_connections, session::kInvalidSessionId, net_context->io_context),
                                                                     });
    if (itr == pending_connections.end()) {
      CORE_LOG_ERROR("Failed to add new connection to client endpoints");
      return;
    }
    ++current_connections;

    asio::ip::tcp::endpoint ep(asio::ip::address_v4(bp.ip), bp.port);
    itr->active_session->socket.async_connect(ep, [this, bp](const asio::error_code& ec) {
      if (!ec) {
        CORE_LOG_INFO("Successfully connected to other application at {}", binding_point::write_string(bp));
        // auto conn_itr = std::ranges::find_if(client_endpoints, [&](const auto& pair) { return pair.second.endpoint.ip == bp.ip && pair.second.endpoint.port == bp.port; });
        // OTHER_ASSERT(conn_itr != client_endpoints.end(), "Connection not found for endpoint {}", binding_point::write_string(bp));
        auto conn_itr = std::ranges::find_if(pending_connections, [&](const connection& conn) { return conn.endpoint.ip == bp.ip && conn.endpoint.port == bp.port; });
        OTHER_ASSERT(conn_itr != pending_connections.end(), "Connection not found for endpoint {}", binding_point::write_string(bp));
        conn_itr->active_session->check_in();
      } else {
        CORE_LOG_ERROR("Failed to connect to other application at {}: {}", binding_point::write_string(bp), ec.message());
        auto conn_itr = std::ranges::find_if(pending_connections, [&](const connection& conn) { return conn.endpoint.ip == bp.ip && conn.endpoint.port == bp.port; });
        if (conn_itr != pending_connections.end()) {
          pending_connections.erase(conn_itr);
        }
      }
    });
  }

  void network_thread::handle_control_ping(message&& msg) {
    message pong_msg;
    pong_msg.header = {
      .category = CONTROL,
      .id = PONG,
    };

    integer_t session_id = session::kNetworkThreadSessionId;
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

    for (auto& [id, binding] : udp_binding_map) {
      try {
        binding.stream->shutdown();
        arena_allocator<udp_stream>{}.free(binding.stream);
        arena_allocator<std::mutex>{}.free(binding.mutex);
      } catch (...) {
      }
    }

    for (auto& conn : pending_connections) {
      conn.active_session->shutdown();
    }

    for (auto& [id, conn] : client_endpoints) {
      conn.active_session->shutdown();
    }

    current_state.shutdown_pending = true;
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
    ack_msg.data.push_back(0x01);
    bus.send_message(std::move(ack_msg));
  }

  void network_thread::handle_command_session_connect_to(message&& msg) {
    OTHER_ASSERT(msg.data.size() == sizeof(binding_point), "Invalid session connect to message size");
    const binding_point* bp = reinterpret_cast<const binding_point*>(msg.data.data());

    CORE_LOG_DEBUG("CONNECT TO [{}]", binding_point::write_string(*bp));
    open_session_and_connect_to(*bp);
  }

  void network_thread::handle_command_session_check_in(message&& msg) {
    OTHER_ASSERT(msg.data.size() >= sizeof(integer_t) + sizeof(uint16_t), "Invalid session check-in message size");
    integer_t session_id = *reinterpret_cast<const integer_t*>(msg.data.data());
    uint16_t port = *reinterpret_cast<const uint16_t*>(msg.data.data() + sizeof(integer_t));

    CORE_LOG_DEBUG("CHECK-IN [{} @ {}]", session_id, port);
    open_session_and_check_in_at(session_id, port);
  }

  void network_thread::handle_command_session_tx_message(message&& msg) {
    OTHER_ASSERT(msg.data.size() >= sizeof(integer_t), "Invalid session tx message size");

    auto bytes = std::span(msg.data);
    integer_t session_id = *reinterpret_cast<const integer_t*>(bytes.data());

    auto itr = std::ranges::find_if(client_endpoints, [&](const auto& pair) { return pair.second.session_id == session_id; });
    if (itr == client_endpoints.end()) {
      CORE_LOG_ERROR("Failed to find session ID {} to transmit message", session_id);
      return;
    }

    bytes = bytes.subspan(sizeof(integer_t));
    message_header msg_header = *reinterpret_cast<const message_header*>(bytes.data());

    auto msg_bytes = bytes.subspan(sizeof(message_header));
    CORE_LOG_DEBUG("Transmitting message to session {}: header={}, data_size={}", session_id, msg_header, msg_bytes.size());

    message tx_msg;
    tx_msg.header = msg_header;
    tx_msg.data.append_range(msg_bytes);
    itr->second.active_session->start_write(std::move(tx_msg));
  }

  void network_thread::handle_command_environment_load_scene(message&& msg) {
    OTHER_ASSERT(msg.data.size() >= sizeof(integer_t), "Invalid environment load scene command message size");

    load_empty_scene_command scene_cmd = other_message_spec::parse<load_empty_scene_command>(msg.data);
    if (scene_cmd.session_id_flag == 0x01) {
      integer_t session_id = scene_cmd.session_id;
      CORE_LOG_INFO("Sending load-empty-scene '{}' to session {}", scene_cmd.scene_name, session_id);
      auto itr = std::ranges::find_if(client_endpoints, [&](const auto& pair) { return pair.second.session_id == session_id; });
      if (itr == client_endpoints.end()) {
        CORE_LOG_WARN("No connected session with ID {}, cannot load scene '{}'", session_id, scene_cmd.scene_name);
        return;
      }

      itr->second.active_session->send_and_wait_response(
        std::move(msg), message_header{ ACKNOWLEDGEMENT, ACK }, seconds(9),
        [this, scene_name = scene_cmd.scene_name](message&& resp_msg) {
          message_header* acked_header = reinterpret_cast<message_header*>(resp_msg.data.data());
          CORE_LOG_DEBUG("Received acknowledgment for load-empty-scene '{}'", scene_name);
          CORE_LOG_DEBUG(" - Acked header: {}", *acked_header);
          bus.send_message(std::move(resp_msg));
        }
      );

    } else {
      CORE_LOG_ERROR("Unimplemented use case for load_empty_scene_command without session ID");
    }
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

  void network_thread::handle_request_new_udp_stream_binding(message&& msg) {
    OTHER_ASSERT(msg.data.size() >= sizeof(uint8_t), "Invalid NEW_UDP_STREAM_BINDING request size");

    natural_t binding_id = next_udp_binding_id++;
    natural_t connection_id = current_connections;
    auto [itr, success] = udp_binding_map.emplace(binding_id, udp_binding());
    if (!success) {
      CORE_LOG_ERROR("Failed to create new UDP stream binding");
      return;
    }
    itr->second.udp_binding_id = binding_id;
    itr->second.connection_number = connection_id;
    itr->second.mutex = arena_allocator<std::mutex>{}.allocate();

    asio::ip::udp::endpoint udp_ep;

    if (msg.data.size() > sizeof(uint8_t) && msg.data[0] == 0x01) {
      uint16_t port = *reinterpret_cast<const uint16_t*>(msg.data.data() + sizeof(uint8_t));
      udp_ep = asio::ip::udp::endpoint(asio::ip::address_v4::any(), port);
    }

    itr->second.stream = arena_allocator<udp_stream>{}.allocate(net_context->io_context, udp_ep);
    itr->second.stream->start_read();

    auto local = itr->second.stream->socket.local_endpoint();
    binding_point bp;
    bp.ip = local.address().to_v4().to_uint();
    bp.port = static_cast<uint16_t>(local.port());
    CORE_LOG_INFO("Creating new UDP stream binding to {}", binding_point::write_string(bp));

    itr->second.endpoint = bp;

    ++current_connections;

    message resp_msg;
    resp_msg.header = {
      .category = RESPONSE,
      .id = NEW_UDP_STREAM_BINDING,
    };

    new_udp_stream_binding_response resp;
    resp.handle.stream = itr->second.stream;
    resp.handle.stream_mutex = itr->second.mutex;
    resp.handle.endpoint = itr->second.endpoint;
    resp_msg.data.append_range(resp.as_buffer());

    bus.send_message(std::move(resp_msg));
    CORE_LOG_DEBUG("Sent NEW_UDP_STREAM_BINDING response for binding ID {}", binding_id);
  }

}  // namespace other
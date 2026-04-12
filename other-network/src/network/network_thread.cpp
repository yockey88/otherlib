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
#include "network/udp_stream.hpp"

namespace other {

  void network_thread::report_connection_closed(natural_t connection_id, integer_t session_id) {
    CORE_LOG_DEBUG("Reporting connection {} session {} as closed", connection_id, session_id);
    session_closures.push({ connection_id, session_id });
  }

  void network_thread::report_connection_error(session* cli, const asio::error_code& ec) {
    OTHER_ASSERT(cli != nullptr, "Client pointer is null");
    CORE_LOG_ERROR("Connection error for client {}: {} [@ {}]", cli->session_id, ec.message(), cli->socket.remote_endpoint().address().to_string());
    report_connection_closed(cli->connection_id, cli->session_id);
  }

  void network_thread::report_stream_closed(natural_t connection_id, integer_t stream_id) {
    CORE_LOG_DEBUG("Reporting UDP stream {} on connection {} as closed", stream_id, connection_id);
    stream_closures.push({ connection_id, stream_id });
  }

  void network_thread::report_stream_error(udp_stream* strm, const asio::error_code& ec) {
    OTHER_ASSERT(strm != nullptr, "UDP stream pointer is null");
    CORE_LOG_ERROR("UDP stream error for stream {}: {} [@ TX = {}, RX = {}]", strm->stream_id, ec.message(), binding_point::write_string(strm->endpoint), binding_point::write_string(strm->endpoint));
    report_stream_closed(strm->connection_id, strm->stream_id);
  }

  void network_thread::report_connection_check_in(natural_t connection_id, integer_t session_id) {
    auto itr = std::ranges::find_if(pending_connections, [&](const connection& conn) { return conn.connection_id.connection_number == connection_id; });
    if (itr == pending_connections.end()) {
      CORE_LOG_ERROR("Failed to find connection id to report check in : {}", connection_id);
      return;
    }
    /// this override works because either we opened the session and had it from the start, or the remote session
    ///   set it and this is correct
    itr->connection_id.id = session_id;

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

      notification_session_check_in check_in_msg;
      check_in_msg.session_id = session_id;
      notif_msg.data.append_range(check_in_msg.as_buffer());
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

  void network_thread::handle_session_closures() {
    while (!session_closures.empty()) {
      connection_key id = session_closures.front();
      session_closures.pop();
      handle_session_closed(id);
    }
  }

  void network_thread::handle_stream_closures() {
    while (!stream_closures.empty()) {
      connection_key id = stream_closures.front();
      stream_closures.pop();
      handle_stream_closed(id);
    }
  }

  void network_thread::handle_session_closed(connection_key id) {
    /// \todo: don't erase the connections because they may reconnect, just mark them as closed
    ///           we need to add a mechanism to know when to fully remove them
    auto itr = client_endpoints.find(id.connection_number);
    if (itr != client_endpoints.end()) {
      CORE_LOG_DEBUG("Closing connection [{}] session {}", id.connection_number, id.id);
      itr->second.active_session->finalize();

      {
        message msg;
        msg.header = {
          .category = NOTIFICATION,
          .id = SESSION_CLOSED,
        };

        notification_session_closed closed_msg;
        closed_msg.session_id = id.id;
        msg.data.append_range(closed_msg.as_buffer());
        bus.send_message(std::move(msg));
      }

      client_endpoints.erase(itr);
      --current_connections;

      CORE_LOG_DEBUG("Session [{}] shut down, {} connections still live", id.id, current_connections);
    } else {
      if (auto itr = std::ranges::find_if(pending_connections, [&](const connection& conn) { return conn.connection_id.connection_number == id.connection_number; });
          itr != pending_connections.end()) {
        CORE_LOG_DEBUG("Closing pending connection [{}] session {}", id.connection_number, id.id);
        pending_connections.erase(itr);
        --current_connections;

        CORE_LOG_DEBUG("Pending session [{}] shut down, {} connections still live", id.id, current_connections);
      }
    }
  }

  void network_thread::handle_stream_closed(connection_key id) {
    auto itr = udp_bindings.find(id.connection_number);
    if (itr != udp_bindings.end()) {
      CORE_LOG_DEBUG("Closing UDP stream binding [{}] stream {}", id.connection_number, id.id);

      itr->second.stream = nullptr;

      udp_bindings.erase(itr);
      --current_connections;

      CORE_LOG_DEBUG("UDP stream binding [{}] shut down. {} connections still live", id.id, current_connections);
    }
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
    if (!net_context->io_context.stopped()) {
      net_context->io_context.poll();
    }

    handle_session_closures();
    handle_stream_closures();

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
            case SESSION_LISTEN_FOR: handle_command_session_listen_for(std::move(*msg)); break;
            case SESSION_CONNECT_TO: handle_command_session_connect_to(std::move(*msg)); break;
            case SESSION_CHECK_IN: handle_command_session_check_in(std::move(*msg)); break;
            case SESSION_TX_MESSAGE: handle_command_session_tx_message(std::move(*msg)); break;
            case STREAM_SEND_UDP_DATAGRAM: handle_command_stream_send_udp_datagram(std::move(*msg)); break;
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
        CORE_LOG_ERROR("Error parsing network packet from session {}: {}", conn.connection_id.id, e.what());
      }
    }

    for (auto& [id, conn] : client_endpoints) {
      try {
        conn.active_session->poll();
      } catch (const network_packet_parse_error& e) {
        CORE_LOG_ERROR("Error parsing network packet from session {}: {}", id, e.what());
      }
    }

    for (auto& [binding_id, udp_binding] : udp_bindings) {
      try {
        udp_binding.stream->poll();
      } catch (const network_packet_parse_error& e) {
        CORE_LOG_ERROR("Error parsing UDP packet from stream {}: {}", binding_id, e.what());
      }
    }

    if (current_state.shutdown_pending && (pending_connections.size() + client_endpoints.size() + udp_bindings.size()) == 0) {
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
      CORE_LOG_INFO("Network thread shutdown complete.");
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
                                                                         .connection_id = connection_key{ current_connections, (integer_t)conn_id },
                                                                         .endpoint = binding_point{ socket.remote_endpoint().address().to_v4().to_uint(), static_cast<uint16_t>(socket.remote_endpoint().port()) },
                                                                         .active_session = make_scope<session>(this, current_connections, conn_id, net_context->io_context, std::move(socket)),
                                                                       });
      if (itr == pending_connections.end()) {
        CORE_LOG_ERROR("Failed to add new connection to client endpoints");
        return;
      }
      ++current_connections;

      CORE_LOG_INFO("New session {} accepted, starting initialization...", itr->connection_id);
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
                                                                        .connection_id = connection_key{ connection_number, session_id },
                                                                        .endpoint = binding_point{ 0, port },
                                                                        .active_session = make_scope<session>(this, connection_number, session_id, net_context->io_context),
                                                                      });
    if (!success) {
      CORE_LOG_ERROR("Failed to add new connection to client endpoints");
      return;
    }
    ++current_connections;

    CORE_LOG_DEBUG("Attempting to open check in session {} @ {}", itr->second.connection_id, binding_point::write_string(itr->second.endpoint));

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
                                                                       .connection_id = connection_key{ current_connections, session::kInvalidSessionId },
                                                                       .endpoint = bp,
                                                                       .active_session = make_scope<session>(this, current_connections, session::kInvalidSessionId, net_context->io_context),
                                                                     });
    if (itr == pending_connections.end()) {
      CORE_LOG_ERROR("Failed to add new connection to client endpoints");
      return;
    }
    ++current_connections;

    CORE_LOG_DEBUG("Attempting to open session {} @ {}", itr->connection_id, binding_point::write_string(bp));

    asio::ip::tcp::endpoint ep(asio::ip::address_v4(bp.ip), bp.port);
    itr->active_session->connection_timer.expires_after(seconds(5));
    itr->active_session->connection_timer.async_wait([this, stime = itr->active_session->connection_timer.expiry()](const asio::error_code& ec) {
      if (ec && ec == asio::error::operation_aborted) {
        return;
      }

      if (!ec) {
        auto itr = std::ranges::find_if(pending_connections, [&](const connection& conn) { return conn.active_session->connection_timer.expiry() == stime; });
        if (itr != pending_connections.end()) {
          itr->active_session->socket.close();
        }
      } else {
      }
    });

    itr->active_session->socket.async_connect(ep, [this, bp](const asio::error_code& ec) {
      message ack_msg;
      ack_msg.header = {
        .category = ACKNOWLEDGEMENT,
        .id = ACK,
      };

      acknowledgement ackmsg;
      ackmsg.acked_header = message_header{ .category = COMMAND, .id = SESSION_CONNECT_TO };
      if (!ec) {
        CORE_LOG_INFO("Successfully connected to other application at {}", binding_point::write_string(bp));

        auto conn_itr = std::ranges::find_if(pending_connections, [&](const connection& conn) { return conn.endpoint.ip == bp.ip && conn.endpoint.port == bp.port; });
        OTHER_ASSERT(conn_itr != pending_connections.end(), "Connection not found for endpoint {}", binding_point::write_string(bp));

        conn_itr->active_session->check_in();
        ackmsg.ack_nack = 1;
      } else {
        CORE_LOG_ERROR("Failed to connect to other application at {}: {}", binding_point::write_string(bp), ec.message());

        auto conn_itr = std::ranges::find_if(pending_connections, [&](const connection& conn) { return conn.endpoint.ip == bp.ip && conn.endpoint.port == bp.port; });
        conn_itr->active_session->connection_timer.cancel();
        if (conn_itr != pending_connections.end()) {
          pending_connections.erase(conn_itr);
        }
        ackmsg.ack_nack = 0;
      }

      ack_msg.data.append_range(ackmsg.as_buffer());
      bus.send_message(std::move(ack_msg));
    });
  }

  void network_thread::handle_control_ping(message&& msg) {
    message pong_msg;
    pong_msg.header = {
      .category = CONTROL,
      .id = PONG,
    };

    integer_t session_id = session::kNetworkThreadSessionId;
    control_pong pong;
    // pong.timestamp = get_current_time_milliseconds();
    pong.session_id = session_id;
    pong_msg.data.append_range(pong.as_buffer());

    bus.send_message(std::move(pong_msg));
  }

  void network_thread::handle_command_shutdown_request(message&& msg) {
    CORE_LOG_DEBUG("Received shutdown request, shutting down network thread...");

    try {
      net_context->acceptor.close();
    } catch (const std::exception& e) {
      CORE_LOG_ERROR("Error closing acceptor: {}", e.what());
    }

    for (auto& [id, binding] : udp_bindings) {
      try {
        binding.stream->shutdown();
      } catch (const std::exception& e) {
        CORE_LOG_ERROR("Error shutting down UDP binding {}: {}", id, e.what());
      }
    }

    for (auto& conn : pending_connections) {
      try {
        conn.active_session->shutdown();
      } catch (const std::exception& e) {
        CORE_LOG_ERROR("Error shutting down pending connection [{},{}]: {}", conn.connection_id.connection_number, conn.connection_id.id, e.what());
      }
    }

    for (auto& [id, conn] : client_endpoints) {
      try {
        conn.active_session->shutdown();
      } catch (const std::exception& e) {
        CORE_LOG_ERROR("Error shutting down connection {}: {}", id, e.what());
      }
    }

    current_state.shutdown_pending = true;
  }

  void network_thread::handle_command_session_listen_for(message&& msg) {
    command_session_listen_at listen_cmd = other_message_spec::parse<command_session_listen_at>(msg.data);
    uint16_t port = listen_cmd.address.port;

    net_context->acceptor = asio::ip::tcp::acceptor(net_context->io_context, asio::ip::tcp::endpoint(asio::ip::address_v4::any(), port));
    CORE_LOG_INFO("Network thread listening for incoming connections on port [{}]", binding_point::write_string(listen_cmd.address));
    net_context->acceptor.async_accept([this](asio::error_code ec, asio::ip::tcp::socket socket) {
      accept_connections(std::move(socket), ec);
    });

    message ack_msg;
    ack_msg.header = {
      .category = ACKNOWLEDGEMENT,
      .id = ACK,
    };

    acknowledgement ackmsg;
    ackmsg.acked_header = msg.header;
    ackmsg.ack_nack = 1;
    ack_msg.data.append_range(ackmsg.as_buffer());
    bus.send_message(std::move(ack_msg));
  }

  void network_thread::handle_command_session_connect_to(message&& msg) {
    command_session_connect_to connect_cmd = other_message_spec::parse<command_session_connect_to>(msg.data);
    const binding_point bp = connect_cmd.address;

    CORE_LOG_DEBUG("CONNECT TO [{}]", binding_point::write_string(bp));
    open_session_and_connect_to(bp);
  }

  void network_thread::handle_command_session_check_in(message&& msg) {
    command_session_check_in_at checkin_cmd = other_message_spec::parse<command_session_check_in_at>(msg.data);
    integer_t session_id = checkin_cmd.session_id;
    uint16_t port = checkin_cmd.address.port;

    CORE_LOG_DEBUG("CHECK-IN [{} @ {}]", session_id, port);
    open_session_and_check_in_at(session_id, port);
  }

  void network_thread::handle_command_session_tx_message(message&& msg) {
    command_session_tx_message tx_cmd = other_message_spec::parse<command_session_tx_message>(msg.data);
    integer_t session_id = tx_cmd.session_id;
    message_header msg_header = tx_cmd.msg.header;
    auto msg_bytes = std::span(tx_cmd.msg.data);
    CORE_LOG_DEBUG("Transmitting message to session {}: header={}, data_size={}", session_id, msg_header, msg_bytes.size());

    auto itr = std::ranges::find_if(client_endpoints, [&](const auto& pair) { return pair.second.connection_id.id == session_id; });
    if (itr == client_endpoints.end()) {
      CORE_LOG_WARN("No connected session with ID {}, cannot transmit message", session_id);
      return;
    }

    message tx_msg;
    tx_msg.header = msg_header;
    tx_msg.data.append_range(msg_bytes);
    itr->second.active_session->start_write(std::move(tx_msg));
  }

  void network_thread::handle_command_stream_send_udp_datagram(message&& msg) {
    command_stream_send_udp_datagram stream_cmd = other_message_spec::parse<command_stream_send_udp_datagram>(msg.data);
    integer_t stream_id = stream_cmd.stream_id;

    auto itr = udp_bindings.find(stream_id);
    if (itr == udp_bindings.end()) {
      CORE_LOG_WARN("No UDP stream binding with ID {}, cannot send datagram", stream_id);
      return;
    }

    auto data_gram_bytes = std::span(msg.data).subspan(sizeof(integer_t));
    itr->second.stream->send(data_gram_bytes);
  }

  void network_thread::handle_command_environment_load_scene(message&& msg) {
    command_load_scene scene_cmd = other_message_spec::parse<command_load_scene>(msg.data);
    if (scene_cmd.session_id_flag == 0x01) {
      integer_t session_id = scene_cmd.session_id;
      CORE_LOG_DEBUG("Sending command-load-scene '{}' to session {}", scene_cmd.scene_name, session_id);

      auto itr = std::ranges::find_if(client_endpoints, [&](const auto& pair) { return pair.second.connection_id.id == session_id; });
      if (itr == client_endpoints.end()) {
        CORE_LOG_WARN("No connected session with ID {}, cannot load scene '{}'", session_id, scene_cmd.scene_name);
        return;
      }

      itr->second.active_session->send_and_wait_response(
        std::move(msg), message_header{ ACKNOWLEDGEMENT, ACK }, seconds(9),
        std::bind_front(&network_thread::on_acknowledge_environment_load_scene, this)
      );

    } else {
      CORE_LOG_ERROR("Unimplemented use case for command_load_scene without session ID");
    }
  }

  void network_thread::on_acknowledge_environment_load_scene(message&& msg) {
    acknowledgement ack = other_message_spec::parse<acknowledgement>(msg.data);
    auto acked_header = ack.acked_header;

    CORE_LOG_DEBUG("Received acknowledgment for load-empty-scene");
    CORE_LOG_DEBUG(" - Acked header: {}", acked_header);
    bus.send_message(std::move(msg));
  }

  void network_thread::handle_request_session_check_in(message&& msg) {
    session_check_in_request checkin_req = other_message_spec::parse<session_check_in_request>(msg.data);
    integer_t session_id = checkin_req.session_id;

    auto callback = [&](integer_t session_id) {
      message msg;
      msg.header = {
        .category = RESPONSE,
        .id = SESSION_CHECK_IN,
      };

      session_check_in_response resp;
      resp.session_id = session_id;
      msg.data.append_range(resp.as_buffer());

      bus.send_message(std::move(msg));
    };

    auto [itr, inserted] = check_in_listeners.insert({ session_id, callback });
    CORE_LOG_DEBUG(" - Registered check-in listener for session ID {}", session_id);
  }

  void network_thread::handle_request_new_udp_stream_binding(message&& msg) {
    new_udp_stream_binding_request req = other_message_spec::parse<new_udp_stream_binding_request>(msg.data);
    message resp_msg;
    resp_msg.header = {
      .category = RESPONSE,
      .id = NEW_UDP_STREAM_BINDING,
    };

    if (req.address.port == 0 || req.remote_address.port == 0) {
      CORE_LOG_ERROR("Cannot create UDP stream binding with port 0");
      new_udp_stream_binding_response resp;
      resp.ack_nack = 0;
      resp_msg.data.append_range(resp.as_buffer());
      bus.send_message(std::move(resp_msg));
      return;
    }

    try {
      integer_t binding_id = next_udp_binding_id++;
      natural_t connection_id = current_connections++;

      asio::ip::udp::endpoint endpoint(asio::ip::make_address_v4(req.address.ip), req.address.port);
      asio::ip::udp::endpoint remote_endpoint(asio::ip::make_address_v4(req.remote_address.ip), req.remote_address.port);
      auto [itr, success] = udp_bindings.emplace(binding_id, udp_binding{
                                                               .connection_id = connection_key{ connection_id, binding_id },
                                                               .endpoint = req.address,
                                                               .stream = make_scope<udp_stream>(this, connection_id, binding_id, net_context->io_context, endpoint, remote_endpoint),
                                                             });
      if (!success) {
        CORE_LOG_ERROR("Failed to create new UDP stream binding");
        new_udp_stream_binding_response resp;
        resp.ack_nack = 0;
        resp_msg.data.append_range(resp.as_buffer());
        bus.send_message(std::move(resp_msg));
        return;
      };

      CORE_LOG_DEBUG("Creating new UDP stream {} @ [LOCAL = {}, REMOTE = {}]", itr->second.connection_id, binding_point::write_string(itr->second.stream->endpoint), binding_point::write_string(remote_endpoint));

      new_udp_stream_binding_response resp;
      resp.ack_nack = 1;
      resp.binding_id = itr->first;
      resp_msg.data.append_range(resp.as_buffer());

      bus.send_message(std::move(resp_msg));
      itr->second.stream->start_read();

      CORE_LOG_DEBUG("Sent NEW_UDP_STREAM_BINDING response for binding ID {}", binding_id);
    } catch (const std::exception& e) {
      CORE_LOG_ERROR("Error creating UDP stream binding: {}", e.what());
    } catch (...) {
      CORE_LOG_ERROR("Unknown error creating UDP stream binding");
    }
  }

}  // namespace other
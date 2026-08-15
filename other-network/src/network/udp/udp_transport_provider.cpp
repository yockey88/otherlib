/**
 * \file network/udp/udp_transport_provider.cpp
 **/
#include "network/udp/udp_transport_provider.hpp"

#include "core/profiler.hpp"

#include "network/asio_endpoint.hpp"
#include "network/network_error.hpp"
#include "network/network_thread.hpp"

#include "asio/asio/ip/address.hpp"
#include "message/messages.hpp"

namespace other {

  void udp_transport_provider::on_initialize() {}

  void udp_transport_provider::on_tick() {
    if (pending_destroy.empty()) {
      return;
    }
    std::deque<natural_t> still_pending;
    for (const natural_t socket_id : pending_destroy) {
      auto itr = sockets.find(socket_id);
      if (itr == sockets.end()) {
        continue;
      }
      if (itr->second->recv_in_flight || itr->second->send_in_flight) {
        still_pending.push_back(socket_id);
        continue;
      }
      sockets.erase(itr);
    }
    pending_destroy = std::move(still_pending);
  }

  void udp_transport_provider::on_begin_shutdown() {
    PROFILE_SECTION("udp_transport_provider::on_begin_shutdown");
    ostd::vector<natural_t> ids;
    for (const auto& [id, entry] : sockets) {
      if (!entry->closing) {
        ids.push_back(id);
      }
    }
    for (const natural_t id : ids) {
      teardown_socket(id, connection_close_reason::SHUTDOWN);
    }
  }

  void udp_transport_provider::on_shutdown() {
    pending_destroy.clear();
    sockets.clear();
    conns.clear();
  }

  void udp_transport_provider::on_start_listen(natural_t conn_id, const binding_point& endpoint) {
    PROFILE_SECTION("udp_transport_provider::on_start_listen");
    OTHER_ASSERT(sockets.find(conn_id) == sockets.end(), "UDP socket ID {} is already in use", conn_id);

    scope<socket_entry> entry = make_scope<socket_entry>();
    entry->listener = true;
    try {
      entry->socket = make_scope<asio::ip::udp::socket>(net_io_ref().context, to_asio_endpoint<asio::ip::udp>(endpoint));
    } catch (const asio::system_error& e) {
      throw port_in_use_network_error(std::format("Failed to bind udp at {}:{}: {}", endpoint.ip, endpoint.port, e.what()));
    }

    auto [itr, success] = sockets.emplace(conn_id, std::move(entry));
    OTHER_ASSERT(success, "Failed to store udp listener for endpoint {}:{}", endpoint.ip, endpoint.port);

    host_thread_ref().register_listener_route(conn_id, this, itr->second.get());
    CORE_LOG_TRACE("[UDP {}: LISTEN] {}", conn_id, endpoint);
    begin_receive(conn_id);
  }

  void udp_transport_provider::on_start_connect(natural_t conn_id, const binding_point& endpoint) {
    PROFILE_SECTION("udp_transport_provider::on_start_connect");
    OTHER_ASSERT(sockets.find(conn_id) == sockets.end(), "UDP socket ID {} is already in use", conn_id);

    scope<socket_entry> entry = make_scope<socket_entry>();
    entry->dial_remote = to_asio_endpoint<asio::ip::udp>(endpoint);
    entry->dial_conn = conn_id;
    try {
      /// bind ephemeral; a datagram socket cannot fail to "connect" — a dead remote
      ///  surfaces as handshake/keepalive timeout above (data, not error)
      entry->socket = make_scope<asio::ip::udp::socket>(net_io_ref().context, asio::ip::udp::endpoint(asio::ip::udp::v4(), 0));
    } catch (const asio::system_error& e) {
      throw network_error(std::format("Failed to open udp socket for {}:{}: {}", endpoint.ip, endpoint.port, e.what()));
    }

    auto [itr, success] = sockets.emplace(conn_id, std::move(entry));
    OTHER_ASSERT(success, "Failed to store udp connection for endpoint {}:{}", endpoint.ip, endpoint.port);
    conns[conn_id] = { .socket_id = conn_id, .remote = itr->second->dial_remote };

    host_thread_ref().register_connection_route(conn_id, this, itr->second.get());
    CORE_LOG_TRACE("[UDP {}: CONNECT] {}", conn_id, endpoint);
    begin_receive(conn_id);
    connection_established(conn_id, endpoint);
  }

  void udp_transport_provider::tx_data(natural_t connection_id, ostd::vector<uint8_t>&& data) {
    auto conn_itr = conns.find(connection_id);
    if (conn_itr == conns.end()) {
      CORE_LOG_WARN("Attempted to send data on non-existent udp connection with ID {}", connection_id);
      return;
    }

    if (data.size() > max_datagram_bytes) {
      /// refused, never fragmented: fragmentation over an unreliable channel is a
      ///  protocol problem, deliberately out of transport scope
      const natural_t refusals = oversize_refused.fetch_add(1, std::memory_order_relaxed) + 1;
      CORE_LOG_WARN("[UDP {}] refusing oversize datagram ({} > {} bytes); refusals: {}", connection_id, data.size(), max_datagram_bytes, refusals);
      return;
    }

    auto socket_itr = sockets.find(conn_itr->second.socket_id);
    if (socket_itr == sockets.end() || socket_itr->second->closing) {
      return;
    }

    socket_entry& entry = *socket_itr->second;
    if (entry.send_queue.size() >= kMaxQueuedDatagrams) {
      /// datagram semantics: drop, count, carry on — closing would be tcp thinking
      CORE_LOG_WARN("[UDP {}] send queue full, dropping datagram", connection_id);
      return;
    }
    entry.send_queue.emplace_back(conn_itr->second.remote, std::move(data));
    kick_send(conn_itr->second.socket_id);
  }

  void udp_transport_provider::kick_send(natural_t socket_id) {
    auto itr = sockets.find(socket_id);
    if (itr == sockets.end()) {
      return;
    }
    socket_entry& entry = *itr->second;
    if (entry.send_in_flight || entry.closing || entry.send_queue.empty()) {
      return;
    }

    entry.send_in_flight = true;
    const auto& [remote, payload] = entry.send_queue.front();
    entry.socket->async_send_to(asio::buffer(payload.data(), payload.size()), remote,
                                std::bind_front(&udp_transport_provider::on_sent, this, socket_id));
  }

  void udp_transport_provider::on_sent(natural_t socket_id, const asio::error_code& ec, size_t bytes) {
    auto itr = sockets.find(socket_id);
    if (itr == sockets.end()) {
      return;
    }
    socket_entry& entry = *itr->second;
    entry.send_in_flight = false;
    if (entry.closing) {
      return;
    }

    if (ec && ec != asio::error::operation_aborted) {
      /// send failures on udp are data (icmp noise, transient); drop and continue
      CORE_LOG_TRACE("[UDP {}] send error: {}", socket_id, ec.message());
    }
    if (!entry.send_queue.empty()) {
      entry.send_queue.pop_front();
    }
    kick_send(socket_id);
  }

  void udp_transport_provider::begin_receive(natural_t socket_id) {
    auto itr = sockets.find(socket_id);
    if (itr == sockets.end()) {
      return;
    }
    socket_entry& entry = *itr->second;
    if (entry.recv_in_flight || entry.closing) {
      return;
    }

    entry.recv_in_flight = true;
    entry.socket->async_receive_from(asio::buffer(entry.recv_buffer), entry.recv_from,
                                     std::bind_front(&udp_transport_provider::on_received, this, socket_id));
  }

  void udp_transport_provider::on_received(natural_t socket_id, const asio::error_code& ec, size_t bytes) {
    PROFILE_SECTION("udp_transport_provider::on_received");
    auto itr = sockets.find(socket_id);
    if (itr == sockets.end()) {
      return;
    }
    socket_entry& entry = *itr->second;
    entry.recv_in_flight = false;
    if (entry.closing) {
      return;
    }

    if (ec) {
      if (ec == asio::error::operation_aborted) {
        return;
      }
      /// windows quirk: a previous send to an unreachable port surfaces here as
      ///  connection_reset — datagram sockets shrug and keep receiving
      CORE_LOG_TRACE("[UDP {}] receive error (continuing): {}", socket_id, ec.message());
      begin_receive(socket_id);
      return;
    }

    natural_t conn_id = 0;
    if (entry.listener) {
      auto remote_itr = entry.remotes.find(entry.recv_from);
      if (remote_itr != entry.remotes.end()) {
        conn_id = remote_itr->second;
      } else {
        /// unknown remote: synthesize the connection. no transport handshake exists —
        ///  whether this peer is welcome is the layer above's business
        conn_id = host_thread_ref().generate_connection_id();
        entry.remotes.emplace(entry.recv_from, conn_id);
        conns[conn_id] = { .socket_id = socket_id, .remote = entry.recv_from };
        host_thread_ref().register_connection_route(conn_id, this, &entry);
        CORE_LOG_TRACE("[UDP {}: SYNTH] connection {} for new remote", socket_id, conn_id);
        connection_accepted(socket_id, conn_id, binding_point::from_asio(entry.recv_from.address(), entry.recv_from.port()));
      }
    } else {
      if (entry.recv_from != entry.dial_remote) {
        CORE_LOG_TRACE("[UDP {}] dropping datagram from unexpected remote", socket_id);
        begin_receive(socket_id);
        return;
      }
      conn_id = entry.dial_conn;
    }

    if (bytes > 0) {
      rx_data(conn_id, std::span<const uint8_t>(entry.recv_buffer.data(), bytes));
    }
    begin_receive(socket_id);
  }

  void udp_transport_provider::net_close(natural_t connection_id) {
    PROFILE_SECTION("udp_transport_provider::close");
    auto socket_itr = sockets.find(connection_id);
    if (socket_itr != sockets.end()) {
      teardown_socket(connection_id, connection_close_reason::LOCAL_CLOSE);
      return;
    }

    /// a synthesized connection: erase the mapping and the route — no wire signal
    ///  exists; LINK_BYE above is the goodbye and keepalive reaps the other side
    auto conn_itr = conns.find(connection_id);
    if (conn_itr == conns.end()) {
      CORE_LOG_WARN("Attempted to close non-existent udp connection with ID {}", connection_id);
      return;
    }

    auto owner = sockets.find(conn_itr->second.socket_id);
    if (owner != sockets.end()) {
      owner->second->remotes.erase(conn_itr->second.remote);
    }
    conns.erase(conn_itr);
    connection_socket_closed(connection_id, connection_close_reason::LOCAL_CLOSE);
  }

  void udp_transport_provider::teardown_socket(natural_t socket_id, connection_close_reason reason) {
    auto itr = sockets.find(socket_id);
    if (itr == sockets.end() || itr->second->closing) {
      return;
    }
    socket_entry& entry = *itr->second;
    entry.closing = true;
    /// keep the in-flight front datagram alive for its aborting async_send_to
    if (entry.send_in_flight && !entry.send_queue.empty()) {
      entry.send_queue.erase(entry.send_queue.begin() + 1, entry.send_queue.end());
    } else {
      entry.send_queue.clear();
    }

    asio::error_code ec;
    entry.socket->close(ec);

    if (entry.listener) {
      /// every synthesized connection dies with its socket
      for (const auto& [remote, conn_id] : entry.remotes) {
        conns.erase(conn_id);
        connection_socket_closed(conn_id, reason);
      }
      entry.remotes.clear();
      connection_socket_closed(socket_id, reason);
    } else {
      conns.erase(entry.dial_conn);
      connection_socket_closed(entry.dial_conn, reason);
    }

    destroy_if_idle(socket_id);
  }

  void udp_transport_provider::destroy_if_idle(natural_t socket_id) {
    auto itr = sockets.find(socket_id);
    if (itr == sockets.end()) {
      return;
    }
    if (itr->second->recv_in_flight || itr->second->send_in_flight) {
      pending_destroy.push_back(socket_id);
      return;
    }
    sockets.erase(itr);
  }

  void udp_transport_provider::connection_removed(natural_t connection_id) {
    /// socket-owning ids retire through teardown_socket; synthesized conns have no
    ///  object of their own
  }

}  // namespace other
/**
 * \file network/transport_provider.cpp
 **/
#include "network/transport_provider.hpp"

#include "core/profiler.hpp"

#include "network/network_thread.hpp"

#include "message/messages.hpp"
#include "peer_mesh/packet_sink.hpp"

namespace other {

  natural_t transport_provider::hash_name(std::string_view transport_name) {
    auto lowercase_name = transport_name | std::views::transform([](unsigned char c) { return std::tolower(c); }) | std::ranges::to<std::string>();
    return FNV(lowercase_name);
  }

  natural_t transport_provider::hash() const {
    PROFILE_SECTION("transport_provider::hash");
    std::string n = name();
    if (n.empty()) {
      CORE_LOG_ERROR("Transport provider has an empty name, which is not allowed. Please override the name() method to return a non-empty name.");
      return 0;
    }
    return hash_name(n);
  }

  void transport_provider::initialize(network_thread* host_thread, io* net_io) {
    OTHER_ASSERT(host_thread != nullptr, "Host thread pointer is null when initializing transport provider.");
    OTHER_ASSERT(net_io != nullptr, "Network IO pointer is null when initializing transport provider.");
    OTHER_ASSERT(this->host_thread == nullptr, "Transport provider is already initialized with a host thread.");
    OTHER_ASSERT(this->net_io == nullptr, "Transport provider is already initialized with a network IO context.");

    this->host_thread = host_thread;
    this->net_io = net_io;

    /// fresh lifecycle: a re-initialized provider must not resurrect listeners from a
    ///  previous life (runs on the registering thread — the registry's single writer)
    registered_listeners.reset();
    conn_scoped_listeners.reset();
    rx_holds.clear();

    on_initialize();
  }

  void transport_provider::tick() {
    OTHER_ASSERT(net_io != nullptr, "Network IO pointer is null when ticking transport provider.");
    OTHER_ASSERT(host_thread != nullptr, "Host thread pointer is null when ticking transport provider.");
    sweep_rx_holds();
    on_tick();
  }

  void transport_provider::begin_shutdown() {
    OTHER_ASSERT(net_io != nullptr, "Network IO pointer is null when beginning shutdown of transport provider.");
    OTHER_ASSERT(host_thread != nullptr, "Host thread pointer is null when beginning shutdown of transport provider.");
    on_begin_shutdown();
  }

  void transport_provider::shutdown() {
    OTHER_ASSERT(net_io != nullptr, "Network IO pointer is null when shutting down transport provider.");
    OTHER_ASSERT(host_thread != nullptr, "Host thread pointer is null when shutting down transport provider.");
    on_shutdown();

    /// listener slots left as-is: shutdown may run on the network thread, which must never
    ///  write the registry — entries die with this object or the next initialize() reset
    rx_holds.clear();

    net_io = nullptr;
    host_thread = nullptr;
  }

  void transport_provider::start_listen(natural_t conn_id, const binding_point& endpoint) {
    CORE_LOG_DEBUG("[TRANSPORT {}] Starting to listen at {}", name(), endpoint);
    on_start_listen(conn_id, endpoint);
  }

  void transport_provider::start_connect(natural_t conn_id, const binding_point& endpoint) {
    CORE_LOG_DEBUG("[TRANSPORT {}] Starting to connect to {}", name(), endpoint);
    on_start_connect(conn_id, endpoint);
  }

  bool transport_provider::has_conn_sink(natural_t conn_id) const {
    return conn_scoped_listeners.find_if([&](const conn_sink_binding& b) { return b.conn_id == conn_id; }) != nullptr;
  }

  void transport_provider::deliver_conn_scoped(natural_t conn_id, std::span<const uint8_t> data) {
    conn_scoped_listeners.for_each([&](conn_sink_binding& binding) {
      if (binding.conn_id == conn_id && binding.sink != nullptr) {
        binding.sink->rx_data(conn_id, data);
      }
    });
  }

  void transport_provider::deliver_conn_scoped_opened(natural_t conn_id) {
    conn_scoped_listeners.for_each([&](conn_sink_binding& binding) {
      if (binding.conn_id == conn_id && binding.sink != nullptr) {
        binding.sink->connection_opened(conn_id);
      }
    });
  }

  void transport_provider::begin_rx_hold(natural_t conn_id) {
    if (has_conn_sink(conn_id)) {
      return;
    }
    rx_holds.emplace(conn_id, rx_hold{ .opened_at = std::chrono::steady_clock::now() });
  }

  void transport_provider::flush_rx_hold(natural_t conn_id) {
    auto itr = rx_holds.find(conn_id);
    if (itr == rx_holds.end()) {
      return;
    }
    for (const ostd::vector<uint8_t>& chunk : itr->second.chunks) {
      deliver_conn_scoped(conn_id, chunk);
    }
    rx_holds.erase(itr);
  }

  void transport_provider::sweep_rx_holds() {
    if (rx_holds.empty()) {
      return;
    }
    const auto now = std::chrono::steady_clock::now();
    for (auto itr = rx_holds.begin(); itr != rx_holds.end();) {
      if (has_conn_sink(itr->first)) {
        const natural_t conn_id = itr->first;
        ++itr;
        flush_rx_hold(conn_id);
        continue;
      }
      if (now - itr->second.opened_at > kRxHoldExpiry) {
        if (itr->second.total > 0) {
          CORE_LOG_TRACE("[TRANSPORT {}] rx hold for connection {} expired unclaimed ({} bytes discarded)", name(), itr->first, itr->second.total);
        }
        itr = rx_holds.erase(itr);
        continue;
      }
      ++itr;
    }
  }

  void transport_provider::rx_data(natural_t connection_id, std::span<const uint8_t> data) {
    PROFILE_SECTION("transport_provider::rx_data");
    CORE_LOG_TRACE("[TRANSPORT {}] Received data on connection {}: {} bytes", name(), connection_id, data.size());
    on_rx_data(connection_id, data);

    /// transport-wide taps see the stream exactly as the wire carried it
    registered_listeners.for_each([&](packet_sink& listener) { listener.rx_data(connection_id, data); });

    if (has_conn_sink(connection_id)) {
      /// held prefix first so a just-registered sink sees a gapless ordered stream
      flush_rx_hold(connection_id);
      deliver_conn_scoped(connection_id, data);
      return;
    }

    auto hold = rx_holds.find(connection_id);
    if (hold == rx_holds.end() || hold->second.expired) {
      return;
    }
    if (hold->second.total + data.size() > kRxHoldMaxBytes) {
      CORE_LOG_WARN("[TRANSPORT {}] rx hold overflow on connection {} ({} bytes) — discarding hold", name(), connection_id, hold->second.total + data.size());
      hold->second.chunks.clear();
      hold->second.total = 0;
      hold->second.expired = true;
      return;
    }
    hold->second.chunks.emplace_back(data.begin(), data.end());
    hold->second.total += data.size();
  }

  void transport_provider::connection_accepted(natural_t listener_id, natural_t conn_id, const binding_point& remote) {
    PROFILE_SECTION("transport_provider::connection_accepted");
    CORE_LOG_DEBUG("[TRANSPORT {}] Connection {} accepted on listener {} from {}:{}", name(), conn_id, listener_id, remote.ip, remote.port);
    begin_rx_hold(conn_id);
    on_connection_accepted(listener_id, conn_id, remote);

    registered_listeners.for_each([&](packet_sink& listener) { listener.connection_opened(conn_id); });
    deliver_conn_scoped_opened(conn_id);

    host_thread_ref().notify_connection_opened(conn_id, remote, listener_id, false);
  }

  void transport_provider::connection_established(natural_t conn_id, const binding_point& remote) {
    PROFILE_SECTION("transport_provider::connection_established");
    CORE_LOG_DEBUG("[TRANSPORT {}] Outbound connection {} established to {}:{}", name(), conn_id, remote.ip, remote.port);
    begin_rx_hold(conn_id);
    on_connection_established(conn_id, remote);

    registered_listeners.for_each([&](packet_sink& listener) { listener.connection_opened(conn_id); });
    deliver_conn_scoped_opened(conn_id);

    host_thread_ref().notify_connection_opened(conn_id, remote, 0, true);
  }

  void transport_provider::connection_socket_closed(natural_t connection_id, connection_close_reason reason) {
    PROFILE_SECTION("transport_provider::connection_socket_closed");
    CORE_LOG_DEBUG("[TRANSPORT {}] connection closed: ID {} (reason {})", name(), connection_id, static_cast<uint16_t>(reason));

    on_connection_socket_closed(connection_id, reason);

    registered_listeners.for_each([&](packet_sink& listener) { listener.connection_closed(connection_id); });
    conn_scoped_listeners.for_each([&](conn_sink_binding& binding) {
      if (binding.conn_id == connection_id && binding.sink != nullptr) {
        binding.sink->connection_closed(connection_id);
      }
    });
    rx_holds.erase(connection_id);

    host_thread_ref().retire_connection_route(connection_id);
    host_thread_ref().notify_connection_closed(connection_id, static_cast<uint16_t>(reason));
  }

}  // namespace other
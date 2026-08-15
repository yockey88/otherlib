/**
 * \file network/transport_provider.cpp
 **/
#include "network/transport_provider.hpp"

#include <thread>

#include "core/profiler.hpp"

#include "network/network_thread.hpp"
#include "network/packet_sink.hpp"

#include "message/messages.hpp"

namespace other {

  /// ---------------------------------------------------------------- the seam

  transport_provider::~transport_provider() {
    destroy_all_sinks();
  }

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

  link_sink& transport_provider::create_sink(natural_t conn_id) {
    std::lock_guard lock(sink_mutex);
    auto [itr, inserted] = sinks.emplace(conn_id, make_scope<link_sink>(*this, conn_id));
    OTHER_ASSERT(inserted, "Connection {} already has a link sink.", conn_id);
    return *itr->second;
  }

  link_sink* transport_provider::sink_of(natural_t conn_id) {
    std::lock_guard lock(sink_mutex);
    auto itr = sinks.find(conn_id);
    return itr != sinks.end() ? itr->second.get() : nullptr;
  }

  void transport_provider::release_sink(natural_t conn_id) {
    std::lock_guard lock(sink_mutex);
    auto itr = sinks.find(conn_id);
    if (itr == sinks.end()) {
      return;
    }
    sink_graveyard.emplace_back(reclaim_epoch_now(), std::move(itr->second));
    sinks.erase(itr);
  }

  void transport_provider::sweep_graveyard(bool force) {
    if (sink_graveyard.empty()) {
      return;
    }
    if (force) {
      /// bounded wait: sinks must not outlive the provider that routes into them
      const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(250);
      while (std::chrono::steady_clock::now() < deadline) {
        const bool all_safe = std::ranges::all_of(sink_graveyard, [&](const auto& entry) { return reclaim_safe(entry.first); });
        if (all_safe) {
          sink_graveyard.clear();
          return;
        }
        std::this_thread::yield();
      }
      /// a wedged reader past the deadline: leak the stragglers rather than free
      ///  memory a pump may still touch
      const size_t leaked = std::erase_if(sink_graveyard, [&](const auto& entry) { return reclaim_safe(entry.first); });
      if (!sink_graveyard.empty()) {
        CORE_LOG_ERROR("[TRANSPORT {}] leaking {} link sink(s): reclaim quiescence never arrived", name(), sink_graveyard.size());
        for (auto& entry : sink_graveyard) {
          entry.second.release();
        }
        sink_graveyard.clear();
      }
      (void)leaked;
      return;
    }
    std::erase_if(sink_graveyard, [&](const auto& entry) { return reclaim_safe(entry.first); });
  }

  void transport_provider::destroy_all_sinks() {
    {
      std::lock_guard lock(sink_mutex);
      for (auto& [conn_id, sink] : sinks) {
        sink_graveyard.emplace_back(reclaim_epoch_now(), std::move(sink));
      }
      sinks.clear();
    }
    sweep_graveyard(true);
  }

  bool transport_provider::has_delegate(natural_t listener_id) {
    std::lock_guard lock(accept_mutex);
    return accept_delegates.contains(listener_id);
  }

  void transport_provider::store_delegate(natural_t listener_id, accept_delegate delegate) {
    if (!delegate) {
      return;
    }
    std::lock_guard lock(accept_mutex);
    accept_delegates[listener_id] = std::move(delegate);
  }

  void transport_provider::release_listener(natural_t listener_id) {
    std::lock_guard lock(accept_mutex);
    accept_delegates.erase(listener_id);
  }

  void transport_provider::queue_accept(natural_t listener_id, natural_t conn_id) {
    std::lock_guard lock(accept_mutex);
    pending_accepts.push_back({ listener_id, conn_id });
  }

  void transport_provider::dispatch_accept(natural_t listener_id, natural_t conn_id) {
    accept_delegate delegate;
    {
      std::lock_guard lock(accept_mutex);
      if (auto itr = accept_delegates.find(listener_id); itr != accept_delegates.end()) {
        delegate = itr->second;
      }
    }
    link_sink* sink = sink_of(conn_id);
    if (sink == nullptr) {
      return;
    }
    if (!delegate) {
      /// the listener released between accept and dispatch: nobody will adopt this
      close(conn_id);
      release_sink(conn_id);
      return;
    }
    delegate(*sink, listener_id);
  }

  void transport_provider::maintain() {
    sweep_graveyard(false);

    std::deque<std::pair<natural_t, natural_t>> drained;
    {
      std::lock_guard lock(accept_mutex);
      drained.swap(pending_accepts);
    }
    for (const auto& [listener_id, conn_id] : drained) {
      dispatch_accept(listener_id, conn_id);
    }
  }

  void transport_provider::observers_rx(natural_t conn_id, std::span<const uint8_t> data) {
    registered_listeners.for_each([&](packet_sink& listener) { listener.rx_data(conn_id, data); });
    conn_scoped_listeners.for_each([&](conn_observer& observer) {
      if (observer.conn_id == conn_id && observer.sink != nullptr) {
        observer.sink->rx_data(conn_id, data);
      }
    });
  }

  void transport_provider::observers_opened(natural_t conn_id) {
    registered_listeners.for_each([&](packet_sink& listener) { listener.connection_opened(conn_id); });
    conn_scoped_listeners.for_each([&](conn_observer& observer) {
      if (observer.conn_id == conn_id && observer.sink != nullptr) {
        observer.sink->connection_opened(conn_id);
      }
    });
  }

  void transport_provider::observers_closed(natural_t conn_id) {
    registered_listeners.for_each([&](packet_sink& listener) { listener.connection_closed(conn_id); });
    conn_scoped_listeners.for_each([&](conn_observer& observer) {
      if (observer.conn_id == conn_id && observer.sink != nullptr) {
        observer.sink->connection_closed(conn_id);
      }
    });
  }

  void transport_provider::reset_observer_registries() {
    registered_listeners.reset();
    conn_scoped_listeners.reset();
  }

  /// ---------------------------------------------------------------- socket engine room

  void socket_transport_provider::attach_main(network_thread* bus) {
    OTHER_ASSERT(bus != nullptr, "Cannot attach a null network thread.");
    main_bus = bus;
  }

  void socket_transport_provider::send_command(uint16_t id, ostd::vector<uint8_t>&& data) {
    message msg(COMMAND, id);
    msg.data = std::move(data);
    main_bus->get_message_bus().send_message(std::move(msg));
  }

  natural_t socket_transport_provider::dial(const net_address& remote) {
    if (remote.addressing != net_address::kind::IP) {
      CORE_LOG_WARN("[TRANSPORT {}] dial refused: address kind is not IP", name());
      return 0;
    }
    if (main_bus == nullptr) {
      CORE_LOG_WARN("[TRANSPORT {}] dial refused: provider is not attached to a network thread", name());
      return 0;
    }

    const natural_t conn_id = main_bus->generate_connection_id();
    create_sink(conn_id);

    command_connect_connection request{
      .endpoint = remote.ip,
      .connection_id = conn_id,
      .transport_hash = hash(),
    };
    send_command(CONNECT_CONNECTION, serialize_direct(request));
    return conn_id;
  }

  natural_t socket_transport_provider::listen(const net_address& bind_addr, accept_delegate on_accept) {
    if (bind_addr.addressing != net_address::kind::IP) {
      CORE_LOG_WARN("[TRANSPORT {}] listen refused: address kind is not IP", name());
      return 0;
    }
    if (main_bus == nullptr) {
      CORE_LOG_WARN("[TRANSPORT {}] listen refused: provider is not attached to a network thread", name());
      return 0;
    }

    const natural_t listener_id = main_bus->generate_connection_id();
    store_delegate(listener_id, std::move(on_accept));

    command_listen_connection request{
      .endpoint = bind_addr.ip,
      .connection_id = listener_id,
      .transport_hash = hash(),
    };
    send_command(LISTEN_CONNECTION, serialize_direct(request));
    return listener_id;
  }

  void socket_transport_provider::tx(natural_t conn_id, std::span<const uint8_t> bytes) {
    if (main_bus == nullptr) {
      return;
    }
    command_tx_data request{
      .connection_id = conn_id,
      .data = ostd::vector<uint8_t>(bytes.begin(), bytes.end()),
    };
    send_command(TX_DATA, serialize_direct(request));
  }

  void socket_transport_provider::close(natural_t conn_id) {
    if (main_bus == nullptr) {
      return;
    }
    command_close_connection request{
      .connection_id = conn_id,
      .transport_hash = hash(),
    };
    send_command(CLOSE_CONNECTION, serialize_direct(request));
  }

  uint64_t socket_transport_provider::reclaim_epoch_now() {
    return main_bus != nullptr ? main_bus->reclamation_epoch() : 0;
  }

  bool socket_transport_provider::reclaim_safe(uint64_t marked) {
    if (main_bus == nullptr || !main_bus->is_running()) {
      return true;
    }
    return main_bus->reclamation_epoch() > marked + 1;
  }

  void socket_transport_provider::initialize(network_thread* thread, io* net) {
    OTHER_ASSERT(thread != nullptr, "Host thread pointer is null when initializing transport provider.");
    OTHER_ASSERT(net != nullptr, "Network IO pointer is null when initializing transport provider.");
    OTHER_ASSERT(this->host_thread == nullptr, "Transport provider is already initialized with a host thread.");
    OTHER_ASSERT(this->net_io == nullptr, "Transport provider is already initialized with a network IO context.");

    this->host_thread = thread;
    this->net_io = net;

    /// fresh lifecycle: a re-initialized provider must not resurrect listeners from a
    ///  previous life (runs on the registering thread — the registry's single writer)
    reset_observer_registries();

    on_initialize();
  }

  void socket_transport_provider::tick() {
    OTHER_ASSERT(net_io != nullptr, "Network IO pointer is null when ticking transport provider.");
    OTHER_ASSERT(host_thread != nullptr, "Host thread pointer is null when ticking transport provider.");
    on_tick();
  }

  void socket_transport_provider::begin_shutdown() {
    OTHER_ASSERT(net_io != nullptr, "Network IO pointer is null when beginning shutdown of transport provider.");
    OTHER_ASSERT(host_thread != nullptr, "Host thread pointer is null when beginning shutdown of transport provider.");
    on_begin_shutdown();
  }

  void socket_transport_provider::shutdown() {
    OTHER_ASSERT(net_io != nullptr, "Network IO pointer is null when shutting down transport provider.");
    OTHER_ASSERT(host_thread != nullptr, "Host thread pointer is null when shutting down transport provider.");
    on_shutdown();

    /// listener slots left as-is: shutdown may run on the network thread, which must never
    ///  write the registry — entries die with this object or the next initialize() reset
    net_io = nullptr;
    host_thread = nullptr;
  }

  void socket_transport_provider::start_listen(natural_t conn_id, const binding_point& endpoint) {
    CORE_LOG_DEBUG("[TRANSPORT {}] Starting to listen at {}", name(), endpoint);
    on_start_listen(conn_id, endpoint);
  }

  void socket_transport_provider::start_connect(natural_t conn_id, const binding_point& endpoint) {
    CORE_LOG_DEBUG("[TRANSPORT {}] Starting to connect to {}", name(), endpoint);
    on_start_connect(conn_id, endpoint);
  }

  void socket_transport_provider::rx_data(natural_t connection_id, std::span<const uint8_t> data) {
    PROFILE_SECTION("transport_provider::rx_data");
    CORE_LOG_TRACE("[TRANSPORT {}] Received data on connection {}: {} bytes", name(), connection_id, data.size());
    on_rx_data(connection_id, data);

    /// observers see the stream exactly as the wire carried it
    observers_rx(connection_id, data);

    if (link_sink* sink = sink_of(connection_id); sink != nullptr) {
      sink->rx_data(connection_id, data);
    }
  }

  void socket_transport_provider::connection_accepted(natural_t listener_id, natural_t conn_id, const binding_point& remote) {
    PROFILE_SECTION("transport_provider::connection_accepted");
    CORE_LOG_DEBUG("[TRANSPORT {}] Connection {} accepted on listener {} from {}:{}", name(), conn_id, listener_id, remote.ip, remote.port);
    on_connection_accepted(listener_id, conn_id, remote);

    /// the sink exists before any byte of this connection can deliver — nothing
    ///  needs holding or replaying
    if (has_delegate(listener_id)) {
      create_sink(conn_id);
      queue_accept(listener_id, conn_id);
    }

    observers_opened(conn_id);
    host_thread_ref().notify_connection_opened(conn_id, remote, listener_id, false);
  }

  void socket_transport_provider::connection_established(natural_t conn_id, const binding_point& remote) {
    PROFILE_SECTION("transport_provider::connection_established");
    CORE_LOG_DEBUG("[TRANSPORT {}] Outbound connection {} established to {}:{}", name(), conn_id, remote.ip, remote.port);
    on_connection_established(conn_id, remote);

    if (link_sink* sink = sink_of(conn_id); sink != nullptr) {
      sink->connection_opened(conn_id);
    }

    observers_opened(conn_id);
    host_thread_ref().notify_connection_opened(conn_id, remote, 0, true);
  }

  void socket_transport_provider::connection_socket_closed(natural_t connection_id, connection_close_reason reason) {
    PROFILE_SECTION("transport_provider::connection_socket_closed");
    CORE_LOG_DEBUG("[TRANSPORT {}] connection closed: ID {} (reason {})", name(), connection_id, static_cast<uint16_t>(reason));

    on_connection_socket_closed(connection_id, reason);

    observers_closed(connection_id);
    if (link_sink* sink = sink_of(connection_id); sink != nullptr) {
      sink->connection_closed(connection_id);
    }

    host_thread_ref().retire_connection_route(connection_id);
    host_thread_ref().notify_connection_closed(connection_id, static_cast<uint16_t>(reason));
  }

}  // namespace other

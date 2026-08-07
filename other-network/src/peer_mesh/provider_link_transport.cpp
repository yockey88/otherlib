/**
 * \file peer_mesh/provider_link_transport.cpp
 **/
#include "peer_mesh/provider_link_transport.hpp"

#include <thread>

#include "core/logger.hpp"
#include "core/profiler.hpp"

namespace other {

  provider_link_transport::provider_link_transport(network_thread& thread, transport_provider& provider, const link_caps& caps, bool stream)
      : thread(thread), provider(provider), default_caps(caps), stream(stream), sink(*this) {
    for (const char c : provider.name()) {
      lowered_name.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    }
  }

  provider_link_transport::~provider_link_transport() {
    for (auto& [conn_id, binding] : live_bindings) {
      provider.unregister_conn_sink(binding.get());
      binding_graveyard.emplace_back(thread.reclamation_epoch(), std::move(binding));
    }
    live_bindings.clear();
    sweep_graveyard(true);
  }

  void provider_link_transport::push_event(rx_event&& ev) {
    std::lock_guard lock(queue_mutex);
    queue.push_back(std::move(ev));
  }

  void provider_link_transport::send_command(uint16_t id, ostd::vector<uint8_t>&& data) {
    message msg(COMMAND, id);
    msg.data = std::move(data);
    thread.get_message_bus().send_message(std::move(msg));
  }

  natural_t provider_link_transport::dial(const net_address& remote) {
    if (remote.addressing != net_address::kind::IP) {
      CORE_LOG_WARN("[LINK-TRANSPORT {}] dial refused: address kind is not IP", lowered_name);
      return 0;
    }

    const natural_t conn_id = thread.generate_connection_id();
    adopt(conn_id, conn_state::DIALING);

    command_connect_connection request{
      .endpoint = remote.ip,
      .connection_id = conn_id,
      .transport_hash = provider.hash(),
    };
    send_command(CONNECT_CONNECTION, serialize_direct(request));
    return conn_id;
  }

  natural_t provider_link_transport::listen(const net_address& bind_addr) {
    if (bind_addr.addressing != net_address::kind::IP) {
      CORE_LOG_WARN("[LINK-TRANSPORT {}] listen refused: address kind is not IP", lowered_name);
      return 0;
    }

    const natural_t listener_id = thread.generate_connection_id();
    my_listeners.insert(listener_id);

    command_listen_connection request{
      .endpoint = bind_addr.ip,
      .connection_id = listener_id,
      .transport_hash = provider.hash(),
    };
    send_command(LISTEN_CONNECTION, serialize_direct(request));
    return listener_id;
  }

  void provider_link_transport::tx(natural_t conn_id, std::span<const uint8_t> bytes) {
    auto itr = states.find(conn_id);
    if (itr == states.end() || itr->second == conn_state::DEAD) {
      return;
    }

    command_tx_data request{
      .connection_id = conn_id,
      .data = ostd::vector<uint8_t>(bytes.begin(), bytes.end()),
    };
    send_command(TX_DATA, serialize_direct(request));
  }

  void provider_link_transport::close(natural_t conn_id) {
    const bool listener = my_listeners.erase(conn_id) > 0;
    auto itr = states.find(conn_id);
    if (!listener && (itr == states.end() || itr->second == conn_state::DEAD)) {
      return;
    }

    command_close_connection request{
      .connection_id = conn_id,
      .transport_hash = provider.hash(),
    };
    send_command(CLOSE_CONNECTION, serialize_direct(request));
  }

  void provider_link_transport::adopt(natural_t conn_id, conn_state initial) {
    scope<conn_sink_binding> binding = make_scope<conn_sink_binding>(conn_id, &sink);
    provider.register_conn_sink(binding.get());
    live_bindings.emplace(conn_id, std::move(binding));
    states[conn_id] = initial;
  }

  void provider_link_transport::retire(natural_t conn_id) {
    auto binding = live_bindings.find(conn_id);
    if (binding != live_bindings.end()) {
      provider.unregister_conn_sink(binding->second.get());
      binding_graveyard.emplace_back(thread.reclamation_epoch(), std::move(binding->second));
      live_bindings.erase(binding);
    }
    states.erase(conn_id);
  }

  void provider_link_transport::sweep_graveyard(bool force) {
    if (binding_graveyard.empty()) {
      return;
    }

    if (force && thread.is_running()) {
      /// bounded wait: bindings must not outlive us (the sink they point at dies here)
      const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(250);
      const uint64_t recorded = thread.reclamation_epoch();
      while (thread.is_running() && thread.reclamation_epoch() <= recorded + 1 &&
             std::chrono::steady_clock::now() < deadline) {
        std::this_thread::yield();
      }
      binding_graveyard.clear();
      return;
    }

    const uint64_t epoch = thread.reclamation_epoch();
    const bool running = thread.is_running();
    std::erase_if(binding_graveyard, [&](const auto& entry) { return !running || epoch > entry.first + 1; });
  }

  void provider_link_transport::on_connection_opened(const notification_connection_opened& note) {
    if (states.contains(note.connection_id)) {
      /// our own dial: the ordered OPENED marker from the sink is authoritative
      return;
    }
    if (note.outbound != 0 || !my_listeners.contains(note.listener_id)) {
      return;
    }

    /// an accept on one of our listeners: adopt it. bytes that raced ahead of this
    ///  notification sit in the provider's rx hold and replay through the sink the
    ///  moment the binding registers — nothing is lost, nothing reorders
    adopt(note.connection_id, conn_state::OPEN);
    CORE_LOG_TRACE("[LINK-TRANSPORT {}] adopted accepted connection {} (listener {})", lowered_name, note.connection_id, note.listener_id);
    if (consumer.accepted) {
      consumer.accepted(note.listener_id, note.connection_id);
    }
  }

  void provider_link_transport::on_connection_closed(const notification_connection_closed& note) {
    auto itr = states.find(note.connection_id);
    if (itr == states.end()) {
      return;
    }
    if (itr->second == conn_state::DIALING) {
      /// dial that never established: no sink events exist, the notification is the
      ///  whole story (reason rides it — CONNECT_FAILED, or a raced close)
      const natural_t conn_id = note.connection_id;
      retire(conn_id);
      if (consumer.closed) {
        consumer.closed(conn_id);
      }
    }
    /// OPEN connections resolve through the ordered CLOSED marker instead
  }

  bool provider_link_transport::handle_bus_message(const message& msg) {
    if (msg.category != NOTIFICATION) {
      return false;
    }
    switch (msg.id) {
      case CONNECTION_OPENED:
        on_connection_opened(deserialize_direct<notification_connection_opened>(msg.data).first);
        return true;
      case CONNECTION_CLOSED:
        on_connection_closed(deserialize_direct<notification_connection_closed>(msg.data).first);
        return true;
      default:
        return false;
    }
  }

  void provider_link_transport::pump() {
    PROFILE_SECTION("provider_link_transport::pump");
    sweep_graveyard(false);

    std::deque<rx_event> drained;
    {
      std::lock_guard lock(queue_mutex);
      drained.swap(queue);
    }

    for (rx_event& ev : drained) {
      auto itr = states.find(ev.conn_id);
      const conn_state state = itr != states.end() ? itr->second : conn_state::DEAD;

      switch (ev.k) {
        case rx_event::kind::OPENED:
          if (state == conn_state::DIALING) {
            itr->second = conn_state::OPEN;
            if (consumer.opened) {
              consumer.opened(ev.conn_id);
            }
          }
          break;

        case rx_event::kind::BYTES:
          if (state == conn_state::OPEN && consumer.received) {
            consumer.received(ev.conn_id, ev.bytes);
          }
          break;

        case rx_event::kind::CLOSED:
          if (itr != states.end() && state != conn_state::DEAD) {
            retire(ev.conn_id);
            if (consumer.closed) {
              consumer.closed(ev.conn_id);
            }
          } else if (itr != states.end()) {
            states.erase(itr);
          }
          break;
      }
    }
  }

}  // namespace other
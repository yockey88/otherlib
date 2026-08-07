/**
 * \file network/memory/memory_fabric.cpp
 **/
#include "network/memory/memory_fabric.hpp"

#include <algorithm>

#include "core/logger.hpp"

namespace other {

  uint64_t memory_fabric::splitmix64::next() {
    state += 0x9E3779B97F4A7C15ull;
    uint64_t z = state;
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
    return z ^ (z >> 31);
  }

  double memory_fabric::splitmix64::next_double() {
    return static_cast<double>(next() >> 11) * 0x1.0p-53;
  }

  void memory_fabric::configure_endpoint(uint64_t endpoint_id, bool datagram) {
    endpoints[endpoint_id].datagram = datagram;
  }

  natural_t memory_fabric::listen(fabric_port& port, const net_address& bind_addr) {
    if (bind_addr.addressing != net_address::kind::MEMORY) {
      return 0;
    }
    endpoint& ep = endpoints[bind_addr.id];
    if (ep.listener_id != 0) {
      CORE_LOG_WARN("[FABRIC] endpoint {} already has a listener", bind_addr.id);
      return 0;
    }
    ep.listener_id = next_id++;
    ep.owner = &port;
    return ep.listener_id;
  }

  natural_t memory_fabric::dial(fabric_port& port, const net_address& remote) {
    if (remote.addressing != net_address::kind::MEMORY) {
      return 0;
    }
    auto itr = endpoints.find(remote.id);
    if (itr == endpoints.end() || itr->second.listener_id == 0 || itr->second.owner == nullptr) {
      return 0;
    }

    const bool datagram = itr->second.datagram;
    const natural_t dial_conn = next_id++;
    const natural_t accept_conn = next_id++;

    connection a{ .peer_conn = accept_conn, .datagram = datagram, .owner = &port };
    a.out.prng.state = channel_seed(a.out.profile, dial_conn);
    connection b{ .peer_conn = dial_conn, .datagram = datagram, .owner = itr->second.owner };
    b.out.prng.state = channel_seed(b.out.profile, accept_conn);
    connections.emplace(dial_conn, std::move(a));
    connections.emplace(accept_conn, std::move(b));

    events.push_back({ .kind = event_kind::ACCEPTED, .target = itr->second.owner, .a = itr->second.listener_id, .b = accept_conn });
    events.push_back({ .kind = event_kind::OPENED, .target = &port, .a = dial_conn });
    return dial_conn;
  }

  link_caps memory_fabric::conn_caps(natural_t conn_id) const {
    auto itr = connections.find(conn_id);
    if (itr == connections.end()) {
      return {};
    }
    return {
      .reliable = !itr->second.datagram,
      .ordered = !itr->second.datagram,
      .max_frame_size = 0,
    };
  }

  uint64_t memory_fabric::channel_seed(const link_profile& profile, natural_t conn_id) const {
    if (profile.seed != 0) {
      return profile.seed;
    }
    return default_seed ^ (conn_id * 0x9E3779B97F4A7C15ull);
  }

  void memory_fabric::set_profile(natural_t conn_id, const link_profile& to_remote, const link_profile& to_local) {
    auto itr = connections.find(conn_id);
    if (itr == connections.end()) {
      return;
    }
    itr->second.out.profile = to_remote;
    itr->second.out.prng.state = channel_seed(to_remote, conn_id);

    auto peer = connections.find(itr->second.peer_conn);
    if (peer != connections.end()) {
      peer->second.out.profile = to_local;
      peer->second.out.prng.state = channel_seed(to_local, itr->second.peer_conn);
    }
  }

  void memory_fabric::set_mute(natural_t conn_id, bool to_remote, bool to_local) {
    auto itr = connections.find(conn_id);
    if (itr == connections.end()) {
      return;
    }
    itr->second.out.muted = to_remote;
    if (auto peer = connections.find(itr->second.peer_conn); peer != connections.end()) {
      peer->second.out.muted = to_local;
    }
  }

  const memory_fabric::channel_stats* memory_fabric::stats_of(natural_t conn_id) const {
    auto itr = connections.find(conn_id);
    return itr != connections.end() ? &itr->second.out.stats : nullptr;
  }

  void memory_fabric::enqueue(connection& conn, std::span<const uint8_t> bytes) {
    channel& ch = conn.out;
    const link_profile& profile = ch.profile;

    if (ch.muted) {
      ch.stats.lost++;
      return;
    }

    const auto jitter_draw = [&]() -> microseconds {
      if (profile.jitter.count() == 0) {
        return microseconds{ 0 };
      }
      const double u = ch.prng.next_double() * 2.0 - 1.0;
      return microseconds{ static_cast<int64_t>(u * static_cast<double>(profile.jitter.count())) };
    };

    /// datagram-only shaping (D11): reliable channels ignore loss/dup/reorder
    natural_t copies = 1;
    if (conn.datagram) {
      if (profile.loss > 0.0f && ch.prng.next_double() < profile.loss) {
        ch.stats.lost++;
        return;
      }
      if (profile.duplicate > 0.0f && ch.prng.next_double() < profile.duplicate) {
        ch.stats.duplicated++;
        copies = 2;
      }
    }

    for (natural_t i = 0; i < copies; ++i) {
      microseconds deliver_at = current_now + profile.latency + jitter_draw();
      if (deliver_at < current_now) {
        deliver_at = current_now;
      }

      if (!conn.datagram) {
        /// order preserved by construction
        deliver_at = std::max(deliver_at, ch.prev_deliver_at);
      } else if (profile.reorder > 0.0f && ch.prng.next_double() < profile.reorder) {
        /// explicit shuffle chance: jump the queue
        deliver_at = current_now;
      }

      if (profile.bandwidth > 0) {
        /// arrival = when the last byte clears the channel; queued frames serialize
        const int64_t transmit_us = static_cast<int64_t>((bytes.size() * 8ull * 1'000'000ull) / profile.bandwidth);
        deliver_at = std::max(deliver_at, ch.bandwidth_free_at) + microseconds{ transmit_us };
        ch.bandwidth_free_at = deliver_at;
      }

      if (!conn.datagram) {
        ch.prev_deliver_at = deliver_at;
      }

      ch.queue.push_back({
        .deliver_at = deliver_at,
        .seq = next_seq++,
        .bytes = ostd::vector<uint8_t>(bytes.begin(), bytes.end()),
      });
    }
  }

  void memory_fabric::tx(natural_t conn_id, std::span<const uint8_t> bytes) {
    auto itr = connections.find(conn_id);
    if (itr == connections.end() || !itr->second.open) {
      return;
    }
    auto peer = connections.find(itr->second.peer_conn);
    if (peer == connections.end() || !peer->second.open) {
      return;
    }
    enqueue(itr->second, bytes);
  }

  void memory_fabric::close(natural_t conn_id) {
    auto itr = connections.find(conn_id);
    if (itr == connections.end() || !itr->second.open) {
      return;
    }
    itr->second.open = false;
    itr->second.out.queue.clear();

    auto peer = connections.find(itr->second.peer_conn);
    if (peer != connections.end() && peer->second.open) {
      peer->second.open = false;
      peer->second.out.queue.clear();
      events.push_back({ .kind = event_kind::CLOSED, .target = peer->second.owner, .a = itr->second.peer_conn });
    }
  }

  void memory_fabric::tick(microseconds now) {
    current_now = now;

    /// establishment/teardown events first, fifo
    while (!events.empty()) {
      const event e = events.front();
      events.pop_front();
      if (e.target == nullptr) {
        continue;
      }
      switch (e.kind) {
        case event_kind::OPENED:
          if (e.target->sink.opened) {
            e.target->sink.opened(e.a);
          }
          break;
        case event_kind::ACCEPTED:
          if (e.target->sink.accepted) {
            e.target->sink.accepted(e.a, e.b);
          }
          break;
        case event_kind::CLOSED:
          if (e.target->sink.closed) {
            e.target->sink.closed(e.a);
          }
          break;
      }
    }

    /// snapshot due frames before delivering — frames enqueued by delivery callbacks
    ///  wait for the next tick, so seeded runs replay exactly and echoes cannot spin
    struct due_frame {
      fabric_port* target = nullptr;
      natural_t to_conn = 0;
      natural_t from_conn = 0;
      microseconds deliver_at{ 0 };
      natural_t seq = 0;
      ostd::vector<uint8_t> bytes;
    };
    ostd::vector<due_frame> due;

    for (auto& [conn_id, conn] : connections) {
      channel& ch = conn.out;
      auto splice_end = std::partition(ch.queue.begin(), ch.queue.end(),
                                       [&](const pending_frame& f) { return f.deliver_at <= now; });
      for (auto itr = ch.queue.begin(); itr != splice_end; ++itr) {
        auto peer = connections.find(conn.peer_conn);
        if (peer == connections.end() || !peer->second.open) {
          continue;
        }
        due.push_back({
          .target = peer->second.owner,
          .to_conn = conn.peer_conn,
          .from_conn = conn_id,
          .deliver_at = itr->deliver_at,
          .seq = itr->seq,
          .bytes = std::move(itr->bytes),
        });
      }
      ch.queue.erase(ch.queue.begin(), splice_end);
    }

    std::ranges::sort(due, [](const due_frame& a, const due_frame& b) {
      return a.deliver_at != b.deliver_at ? a.deliver_at < b.deliver_at : a.seq < b.seq;
    });

    for (due_frame& frame : due) {
      auto to = connections.find(frame.to_conn);
      if (to == connections.end() || !to->second.open || frame.target == nullptr) {
        continue;
      }
      if (auto from = connections.find(frame.from_conn); from != connections.end()) {
        from->second.out.stats.delivered++;
      }
      if (frame.target->sink.received) {
        frame.target->sink.received(frame.to_conn, frame.bytes);
      }
    }
  }

}  // namespace other

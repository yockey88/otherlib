/**
 * \file network/memory/memory_fabric.hpp
 **/
#ifndef OTHER_NETWORK_NETWORK_MEMORY_MEMORY_FABRIC_HPP
#define OTHER_NETWORK_NETWORK_MEMORY_MEMORY_FABRIC_HPP

#include <deque>

#include "core/defines.hpp"
#include "core/time.hpp"

#include "peer_mesh/link_transport.hpp"

namespace other {

  class fabric_port;

  /// the shared simulated medium: pairs endpoints, shapes per-direction channels, and
  ///  delivers on tick. meshes attach through fabric_ports (one port = one consumer);
  ///  both endpoints of a connection may live in the same process — and the same mesh.
  ///  fully deterministic: time is the tick parameter, all randomness comes from
  ///  seeded per-channel prngs — no wall clock, no global rng
  class memory_fabric {
   public:
    struct channel_stats {
      natural_t delivered = 0;
      natural_t lost = 0;
      natural_t duplicated = 0;
    };

    explicit memory_fabric(uint64_t default_seed = 0)
        : default_seed(default_seed) {}

    /// datagram endpoints hand loss/dup/reorder shaping to their channels (D11);
    ///  reliable endpoints keep order by construction. declare before listen/dial
    void configure_endpoint(uint64_t endpoint_id, bool datagram);

    /// per-direction shaping: to_remote = the connection's outbound channel,
    ///  to_local = the peer's outbound channel back
    void set_profile(natural_t conn_id, const link_profile& to_remote, const link_profile& to_local);
    /// silent packet blackhole per direction — the keepalive-timeout test switch
    void set_mute(natural_t conn_id, bool to_remote, bool to_local);

    const channel_stats* stats_of(natural_t conn_id) const;

    /// delivers due frames and queued establishment events; frames enqueued during a
    ///  tick wait for the next one (no same-tick echo loops)
    void tick(microseconds now);

   private:
    friend class fabric_port;

    struct splitmix64 {
      uint64_t state = 0;
      uint64_t next();
      /// uniform [0, 1)
      double next_double();
    };

    struct pending_frame {
      microseconds deliver_at{ 0 };
      natural_t seq = 0;
      ostd::vector<uint8_t> bytes;
    };

    struct channel {
      link_profile profile;
      splitmix64 prng;
      std::deque<pending_frame> queue;
      microseconds prev_deliver_at{ 0 };
      microseconds bandwidth_free_at{ 0 };
      bool muted = false;
      channel_stats stats;
    };

    struct connection {
      natural_t peer_conn = 0;
      bool datagram = false;
      bool open = true;
      fabric_port* owner = nullptr;
      channel out;
    };

    struct endpoint {
      natural_t listener_id = 0;
      bool datagram = false;
      fabric_port* owner = nullptr;
    };

    enum class event_kind : uint8_t { OPENED, ACCEPTED, CLOSED };
    struct event {
      event_kind kind;
      fabric_port* target = nullptr;
      natural_t a = 0;  // listener (ACCEPTED) / conn otherwise
      natural_t b = 0;  // accepted conn
    };

    natural_t listen(fabric_port& port, const net_address& bind_addr);
    natural_t dial(fabric_port& port, const net_address& remote);
    void tx(natural_t conn_id, std::span<const uint8_t> bytes);
    void close(natural_t conn_id);
    link_caps conn_caps(natural_t conn_id) const;

    void enqueue(connection& conn, std::span<const uint8_t> bytes);
    uint64_t channel_seed(const link_profile& profile, natural_t conn_id) const;

    uint64_t default_seed = 0;

    ostd::map<uint64_t, endpoint> endpoints;
    ostd::map<natural_t, connection> connections;
    std::deque<event> events;

    natural_t next_id = 1;
    natural_t next_seq = 1;
    microseconds current_now{ 0 };
  };

  /// one mesh's attachment to the medium — the link_transport a mesh registers.
  ///  simulation shape: one process, one fabric, one port per mesh
  class fabric_port final : public link_transport {
   public:
    explicit fabric_port(memory_fabric& medium)
        : medium(medium) {}

    std::string_view name() const override { return "memory"; }
    bool is_stream() const override { return false; }
    link_caps conn_caps(natural_t conn_id) const override { return medium.conn_caps(conn_id); }

    void bind(callbacks cbs) override { sink = std::move(cbs); }

    natural_t dial(const net_address& remote) override { return medium.dial(*this, remote); }
    natural_t listen(const net_address& bind_addr) override { return medium.listen(*this, bind_addr); }

    void tx(natural_t conn_id, std::span<const uint8_t> bytes) override { medium.tx(conn_id, bytes); }
    void close(natural_t conn_id) override { medium.close(conn_id); }

   private:
    friend class memory_fabric;

    memory_fabric& medium;
    callbacks sink;
  };

}  // namespace other

#endif  // OTHER_NETWORK_NETWORK_MEMORY_MEMORY_FABRIC_HPP

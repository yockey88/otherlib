/**
 * \file network/memory/memory_transport_provider.hpp
 **/
#ifndef OTHER_NETWORK_NETWORK_MEMORY_MEMORY_TRANSPORT_PROVIDER_HPP
#define OTHER_NETWORK_NETWORK_MEMORY_MEMORY_TRANSPORT_PROVIDER_HPP

#include <deque>
#include <functional>

#include "core/defines.hpp"
#include "core/time.hpp"

#include "network/transport_provider.hpp"

namespace other {

  /// per-direction channel shaping, applied by the medium at enqueue; all-zero = passthrough
  struct link_profile {
    microseconds latency{ 0 };
    microseconds jitter{ 0 };

    float loss = 0.0f;
    float duplicate = 0.0f;
    float reorder = 0.0f;

    uint32_t bandwidth = 0;
    uint64_t seed = 0;
  };

  /// the simulated medium as a main-thread provider: pairs endpoints, shapes per-direction
  ///  channels, delivers on tick straight through each connection's link sink. fully
  ///  deterministic — time is the tick param, prngs are seeded per-channel. meshes attach
  ///  this provider directly; several meshes may share one medium
  class memory_transport_provider : public transport_provider {
   public:
    struct channel_stats {
      natural_t delivered = 0;
      natural_t lost = 0;
      natural_t duplicated = 0;
    };

    struct splitmix64 {
      uint64_t state = 0;
      uint64_t next();
      /// uniform [0, 1)
      double next_double();
    };

    /// the simulation seam (D27): a user transform over each send, ahead of the channel's
    ///  profile shaping — rewrite the framed bytes, drop, duplicate, or delay delivery.
    ///  identity (unset) costs nothing; draws taken from the channel prng keep the D16
    ///  determinism contract. reliable channels stay order-preserving regardless
    struct memory_transform_context {
      natural_t conn_id = 0;
      bool datagram = false;
      microseconds now{ 0 };
      ostd::vector<uint8_t>& bytes;
      splitmix64& prng;

      bool drop = false;
      uint32_t copies = 1;
      microseconds delay{ 0 };
    };
    using memory_transform = std::function<void(memory_transform_context&)>;

    explicit memory_transport_provider(uint64_t default_seed = 0)
        : default_seed(default_seed) {}

    std::string name() const override { return "memory"; }
    transport_home execution_home() const override { return transport_home::MAIN_THREAD; }
    bool is_stream() const override { return false; }
    link_caps conn_caps(natural_t conn_id) const override;

    /// datagram endpoints hand loss/dup/reorder shaping to their channels (D11);
    ///  reliable endpoints keep order by construction. declare before listen/dial
    void configure_endpoint(uint64_t endpoint_id, bool datagram);

    natural_t dial(const net_address& remote) override;
    natural_t listen(const net_address& bind_addr, accept_delegate on_accept) override;
    void tx(natural_t conn_id, std::span<const uint8_t> bytes) override;
    void close(natural_t conn_id) override;

    /// per-direction shaping: to_remote = the connection's outbound channel,
    ///  to_local = the peer's outbound channel back
    void set_profile(natural_t conn_id, const link_profile& to_remote, const link_profile& to_local);
    /// silent packet blackhole per direction — the keepalive-timeout test switch
    void set_mute(natural_t conn_id, bool to_remote, bool to_local);

    /// medium-wide transform (every outbound send), and per-connection overrides
    void set_transform(memory_transform fn) { global_transform = std::move(fn); }
    void set_transform(natural_t conn_id, memory_transform fn) { conn_transforms[conn_id] = std::move(fn); }

    const channel_stats* stats_of(natural_t conn_id) const;

    /// delivers due frames and queued establishment events; frames enqueued during a
    ///  tick wait for the next one (no same-tick echo loops)
    void tick(microseconds now);

   private:
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
      /// graceful close: no new tx, in-flight frames still deliver, the CLOSED
      ///  event queues behind them (like a FIN behind flushed data)
      bool closing = false;
      channel out;
    };

    struct endpoint {
      natural_t listener_id = 0;
      bool datagram = false;
    };

    enum class event_kind : uint8_t { OPENED, ACCEPTED, CLOSED };
    struct event {
      event_kind kind;
      natural_t a = 0;  // listener (ACCEPTED) / conn otherwise
      natural_t b = 0;  // accepted conn
    };

    void enqueue(connection& conn, std::span<const uint8_t> bytes, microseconds extra_delay);
    uint64_t channel_seed(const link_profile& profile, natural_t conn_id) const;
    const memory_transform* transform_of(natural_t conn_id) const;

    uint64_t default_seed = 0;
    memory_transform global_transform;
    ostd::map<natural_t, memory_transform> conn_transforms;

    ostd::map<uint64_t, endpoint> endpoints;
    ostd::map<natural_t, connection> connections;
    std::deque<event> events;

    natural_t next_id = 1;
    natural_t next_seq = 1;
    microseconds current_now{ 0 };
  };

}  // namespace other

#endif  // OTHER_NETWORK_NETWORK_MEMORY_MEMORY_TRANSPORT_PROVIDER_HPP

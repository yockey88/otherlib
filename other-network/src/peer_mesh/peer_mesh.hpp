/**
 * \file peer_mesh/peer_mesh.hpp
 **/
#ifndef OTHER_NETWORK_PEER_MESH_PEER_MESH_HPP
#define OTHER_NETWORK_PEER_MESH_PEER_MESH_HPP

#include <functional>
#include <string>

#include "core/defines.hpp"
#include "core/scope.hpp"
#include "core/time.hpp"

#include "network/frame.hpp"
#include "network/link.hpp"
#include "network/link_security.hpp"
#include "network/net_address.hpp"
#include "network/node_id.hpp"

#include "peer_mesh/peer_actor.hpp"

namespace other {

  class transport_provider;
  class link_sink;

  struct peer_mesh_config {
    /// both hellos must agree; 0 = unset (still compared)
    natural_t app_hash = 0;
    natural_t max_links = 32;
    uint32_t max_frame_size = kDefaultMaxFrameSize;
    microseconds handshake_timeout{ 3'000'000 };
    microseconds keepalive_idle{ 5'000'000 };
    microseconds link_timeout{ 15'000'000 };
  };

  /// one logical network: one primary actor (the local endpoint), N secondaries (the
  ///  mesh's records of remote endpoints — inert by default, simulated when explicit),
  ///  and the link sinks that join them. the mesh touches no bytes: sinks interpret,
  ///  the mesh dispatches — a frame on sink S delivers to the actor at S's local seat
  class peer_mesh {
   public:
    explicit peer_mesh(std::string_view debug_name, const peer_mesh_config& config = {});
    peer_mesh(const peer_mesh&) = delete;
    peer_mesh& operator=(const peer_mesh&) = delete;
    ~peer_mesh();

    const std::string& name() const { return debug_name; }
    const peer_mesh_config& config() const { return cfg; }

    /// live links, snapshot by value; link()/link_between() point into sink-owned
    ///  records — stable until that link dies
    ostd::vector<link_record> links() const;
    const link_record* link(natural_t link_id) const;
    const link_record* link_between(node_id local, node_id remote) const;
    size_t link_count() const { return link_sinks.size(); }

    /// not owned; providers outlive the mesh. establishment events fire from the
    ///  provider's tick/pump on the same thread this mesh ticks on
    void attach_provider(transport_provider& provider);
    /// status-facing: the lowered name of the attached provider a link rides ("" = unknown)
    std::string_view transport_name(natural_t transport_hash) const;

    /// the local endpoint — exactly one, spawned before any secondary
    peer_actor& set_primary(scope<peer_actor> actor, node_id id);
    peer_actor* primary() { return primary_id != 0 ? actor(primary_id) : nullptr; }

    /// an explicit secondary: a simulated remote (behavior + seats over memory links)
    ///  or a pre-registered record. real remotes mint their own on link-up
    peer_actor& add_secondary(scope<peer_actor> actor, node_id id);
    /// how link-up materializes an unknown remote; default = an inert record actor.
    ///  returning null skips the mint
    void set_secondary_factory(std::function<scope<peer_actor>(const link_record&)> fn);

    void remove_actor(node_id id);
    peer_actor* actor(node_id id);
    size_t actor_count() const { return resident_actors.size(); }

    void set_security(scope<link_security> new_security);
    link_security* security() { return link_sec.get(); }

    /// drives provider maintenance, sink pumps (handshake/keepalive/timeouts), and actor
    ///  ticks. time is a parameter — the driver passes engine time, sims a virtual clock
    void tick(microseconds now);

    void close_link(natural_t link_id, link_close_reason reason);

    const mesh_counters& counters() const { return stats; }

   private:
    friend class peer_actor;

    struct transport_entry {
      transport_provider* provider = nullptr;
      std::string lowered_name;
    };

    // actor-called (through the actor's public helpers)
    natural_t open_link_from(peer_actor& from, const net_address& remote, std::string_view transport_name);
    natural_t open_listener_from(peer_actor& from, const net_address& bind, std::string_view transport_name);
    bool send_from(peer_actor& from, node_id dst, uint16_t net_id, std::span<const uint8_t> payload);
    bool send_on_link_from(peer_actor& from, natural_t link_id, uint16_t net_id, std::span<const uint8_t> payload);
    ostd::vector<link_record> links_of(node_id local) const;

    // sink adoption + upward events
    natural_t adopt_sink(link_sink& sink, size_t transport_index, node_id local_seat, link_state initial);
    void handle_accept(size_t transport_index, link_sink& sink, natural_t listener_id);
    void on_sink_up(link_record& record);
    void on_sink_down(link_sink& sink, const link_record& snapshot, link_close_reason reason);
    void on_sink_frame(const link_record& record, uint16_t net_id, std::span<const uint8_t> payload);

    bool transmit(natural_t link_id, uint16_t net_id, std::span<const uint8_t> payload);

    peer_actor& spawn_common(scope<peer_actor> actor, node_id id, bool primary, bool auto_spawned);
    void reap_auto_secondary(node_id id);

    link_sink* sink(natural_t link_id);
    const link_sink* sink(natural_t link_id) const;
    natural_t resolve_transport(const net_address& remote, std::string_view transport_name, size_t& out_index);

    std::string debug_name;
    peer_mesh_config cfg;

    ostd::map<natural_t, link_sink*> link_sinks;
    natural_t next_link_id = 1;

    ostd::vector<transport_entry> transports;
    ostd::map<std::pair<size_t, natural_t>, node_id> listener_owners;

    ostd::vector<scope<peer_actor>> resident_actors;
    node_id primary_id = 0;
    std::function<scope<peer_actor>(const link_record&)> secondary_factory;

    scope<link_security> link_sec;

    mesh_counters stats;
    microseconds current_now{ 0 };
    natural_t tick_counter = 0;
    bool ticked_once = false;
  };

}  // namespace other

#endif  // OTHER_NETWORK_PEER_MESH_PEER_MESH_HPP

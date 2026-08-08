/**
 * \file peer_mesh/peer_mesh.hpp
 **/
#ifndef OTHER_NETWORK_PEER_MESH_PEER_MESH_HPP
#define OTHER_NETWORK_PEER_MESH_PEER_MESH_HPP

#include <span>
#include <string>

#include "core/defines.hpp"
#include "core/scope.hpp"
#include "core/time.hpp"

#include "network/net_address.hpp"
#include "peer_mesh/link.hpp"
#include "peer_mesh/link_security.hpp"
#include "peer_mesh/link_transport.hpp"
#include "peer_mesh/mesh_filter.hpp"
#include "peer_mesh/mesh_router.hpp"
#include "peer_mesh/network.hpp"
#include "peer_mesh/node_id.hpp"
#include "peer_mesh/peer_graph.hpp"
#include "peer_mesh/peer_mesh_actor.hpp"

namespace other {

  enum class mesh_message : uint16_t;

  struct peer_mesh_config {
    /// both hellos must agree; 0 = unset (still compared)
    natural_t app_hash = 0;
    natural_t max_links = 32;
    uint32_t max_frame_size = kDefaultMaxFrameSize;
    microseconds handshake_timeout{ 3'000'000 };
    microseconds keepalive_idle{ 5'000'000 };
    microseconds link_timeout{ 15'000'000 };
    uint8_t default_ttl = 8;
  };

  /// manages a network and its peer-mesh actors — the boundary between project and network.
  ///  not a packet sink: bytes arrive only through registered transports, payloads stay opaque
  class peer_mesh {
   public:
    struct mesh_counters {
      natural_t refused_sends = 0;
      natural_t malformed_frames = 0;
      natural_t no_actor_drops = 0;
      natural_t unroutable_drops = 0;
      natural_t security_failures = 0;
      natural_t protocol_errors = 0;
    };

    explicit peer_mesh(std::string_view debug_name, const peer_mesh_config& config = {});
    peer_mesh(const peer_mesh&) = delete;
    peer_mesh& operator=(const peer_mesh&) = delete;
    ~peer_mesh();

    const std::string& name() const { return debug_name; }
    const peer_mesh_config& config() const { return cfg; }
    network& net() { return links; }
    const network& net() const { return links; }
    peer_graph& graph() { return nodes; }

    /// not owned; one mesh per transport instance. events fire from the transport's
    ///  tick/pump, on the same thread this mesh ticks on
    void register_transport(link_transport& transport);
    link_transport* transport(std::string_view transport_name);

    peer_mesh_actor& spawn_actor(scope<peer_mesh_actor> actor, node_id id);
    void destroy_actor(node_id id);
    peer_mesh_actor* actor(node_id id);
    size_t actor_count() const { return resident_actors.size(); }

    void add_rx_filter(mesh_filter fn);
    void add_tx_filter(mesh_filter fn);
    void set_router(scope<mesh_router> new_router);
    mesh_router& router();
    void set_security(scope<link_security> new_security);
    link_security* security() { return link_sec.get(); }

    /// drives handshake timeouts, keepalive, and actor ticks. time is a parameter —
    ///  the driver passes engine time, simulations pass a virtual clock
    void tick(microseconds now);

    void close_link(natural_t link_id, link_close_reason reason);

    /// for link_security implementations during AUTHENTICATING
    void send_link_auth(link_record& link, std::span<const uint8_t> blob);

    const mesh_counters& counters() const { return stats; }

   private:
    friend class peer_mesh_actor;

    struct transport_entry {
      link_transport* transport = nullptr;
    };

    // actor-called (through the actor's public helpers)
    natural_t open_link_from(peer_mesh_actor& from, const net_address& remote, std::string_view transport_name);
    natural_t open_listener_from(peer_mesh_actor& from, const net_address& bind, std::string_view transport_name);
    bool send_from(peer_mesh_actor& from, node_id dst, uint16_t net_id, std::span<const uint8_t> payload);
    bool send_on_link_from(peer_mesh_actor& from, natural_t link_id, uint16_t net_id, std::span<const uint8_t> payload);
    ostd::vector<link_record> links_of(node_id local) const;

    // transport callbacks
    void on_conn_opened(size_t transport_index, natural_t conn_id);
    void on_conn_accepted(size_t transport_index, natural_t listener_id, natural_t conn_id);
    void on_conn_received(size_t transport_index, natural_t conn_id, std::span<const uint8_t> bytes);
    void on_conn_closed(size_t transport_index, natural_t conn_id);

    // pipeline
    void process_frame(natural_t link_id, parsed_frame&& frame);
    void handle_control(natural_t link_id, uint16_t net_id, std::span<const uint8_t> payload);
    void handle_hello(natural_t link_id, std::span<const uint8_t> payload);
    void try_advance_past_handshake(natural_t link_id);
    void apply_auth_result(natural_t link_id, link_security::auth_result result);
    void make_link_up(natural_t link_id);
    bool run_filters(ostd::vector<mesh_filter>& chain, const link_record& link, node_id src, node_id dst,
                     uint16_t net_id, std::span<const uint8_t>& payload, ostd::vector<uint8_t>& scratch);
    void deliver_or_forward(natural_t link_id, node_id src, node_id dst, uint8_t ttl, bool routed,
                            uint16_t net_id, std::span<const uint8_t> payload);
    void forward(natural_t in_link_id, const route_header& route, uint16_t net_id, std::span<const uint8_t> payload);

    // tx
    bool transmit(natural_t link_id, uint16_t net_id, std::span<const uint8_t> payload, const route_header* route);
    void send_control(natural_t link_id, mesh_message id, std::span<const uint8_t> payload);
    void send_hello(natural_t link_id);

    void teardown(natural_t link_id, link_close_reason reason, bool send_bye);
    link_transport* transport_of(natural_t link_id);
    natural_t resolve_transport(const net_address& remote, std::string_view transport_name, size_t& out_index);
    void protocol_error(natural_t link_id, std::string_view what);

    std::string debug_name;
    peer_mesh_config cfg;

    network links;
    peer_graph nodes;

    ostd::vector<transport_entry> transports;
    ostd::map<std::pair<size_t, natural_t>, node_id> listener_owners;

    ostd::vector<scope<peer_mesh_actor>> resident_actors;

    ostd::vector<mesh_filter> rx_filters;
    ostd::vector<mesh_filter> tx_filters;
    scope<mesh_router> active_router;
    scope<link_security> link_sec;

    mesh_counters stats;
    microseconds current_now{ 0 };
    natural_t tick_counter = 0;
    bool ticked_once = false;
  };

}  // namespace other

#endif  // OTHER_NETWORK_PEER_MESH_PEER_MESH_HPP

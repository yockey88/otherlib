/**
 * \file network/session/network_session.hpp
 **/
#ifndef OTHERLIB_NETWORK_SESSION_NETWORK_SESSION_HPP
#define OTHERLIB_NETWORK_SESSION_NETWORK_SESSION_HPP

#include <functional>

#include "core/defines.hpp"
#include "core/scope.hpp"

#include "peer_mesh/peer_mesh_actor.hpp"
#include "peer_mesh/peer_state_machine.hpp"

#include "network/session/net_messages.hpp"

namespace other {

  enum class session_event : uint8_t {
    STARTED,     // arg = local peer id
    ENDED,       // arg = reason (net_reject_reason on reject, else link/leave reason)
    PEER_JOINED, // arg = peer id
    PEER_LEFT,   // arg = peer id
  };

  struct session_member {
    node_id node = 0;
    uint16_t peer_id = 0;
    /// host: the link to that member; client: the host link on entry 0, else 0
    natural_t link_id = 0;
    std::string name;
  };

  /// the shipped default peer_mesh_actor: client-server, host-authoritative, star
  ///  topology — all of it this actor's policy, asserted here and nowhere lower.
  ///  membership and authority only: framing, keepalive, RTT and the live peer
  ///  table already live at the link/mesh level
  class OTHER_CLASS network_session final : public peer_mesh_actor {
   public:
    constexpr static std::string_view kDefaultActorName = "client-server";

    struct session_config {
      uint16_t max_peers = 8;  // total members, host included
      /// JOIN_REQUEST must follow link-up (and WELCOME must follow the request)
      ///  within this window or the link closes
      microseconds join_timeout{ 3'000'000 };
      std::string display_name = "peer";
      /// opaque to the session; a join validator's token channel
      uint32_t client_flags = 0;
    };

    /// accept = nullopt, reject = a net_reject_reason-page value (policy; link-level
    ///  identity already ran below at the security seam)
    using join_validator = std::function<opt<uint16_t>(const link_record&, const net_join_request&)>;
    using session_observer = std::function<void(session_event, uint16_t)>;
    using frame_handler = std::function<void(node_id src, std::span<const uint8_t> payload)>;
    using game_event_handler = std::function<void(uint16_t sender_peer, std::string_view name, std::span<const uint8_t> payload)>;

    explicit network_session(const session_config& cfg = {})
        : cfg(cfg) {}

    std::string_view name() const override { return kDefaultActorName; }

    /// open the listener and promote; false (with a warn) when unspawned, already
    ///  in a session, or the bind is refused
    bool host(const net_address& bind);
    /// dial the host; JOIN_REQUEST fires on link-up, WELCOME completes the join
    bool join(const net_address& remote);
    void leave(uint16_t reason = 0);

    const peer_state_machine& role() const { return state; }
    bool is_host() const { return state.get_current_state() == role_state::SERVER; }
    bool in_session() const { return is_host() || state.get_current_state() == role_state::PEER; }
    uint16_t local_peer_id() const { return local_peer; }
    const ostd::vector<session_member>& peers() const { return members; }

    bool send(uint16_t peer_id, net_message id, std::span<const uint8_t> payload);
    bool broadcast(net_message id, std::span<const uint8_t> payload);  // host only

    bool send_game_event(std::string_view event_name, std::span<const uint8_t> payload);       // client -> host
    bool broadcast_game_event(std::string_view event_name, std::span<const uint8_t> payload);  // host -> all

    /// replication (05) + ops (06) hook: the typed dispatch 01 §7 leaves to concrete
    ///  actors. session control ids and GAME_EVENT are consumed internally
    void register_handler(net_message id, frame_handler fn);
    void set_join_validator(join_validator fn) { validator = std::move(fn); }
    void set_observer(session_observer fn) { observer = std::move(fn); }
    void set_game_event_handler(game_event_handler fn) { on_game_event = std::move(fn); }

    // peer_mesh_actor
    void on_frame(const link_record& via, node_id src, uint16_t net_id, std::span<const uint8_t> payload) override;
    void on_link_up(const link_record& link) override;
    void on_link_down(const link_record& link, link_close_reason reason) override;
    void tick(microseconds now, double dt) override;

   private:
    session_config cfg;
    peer_state_machine state;

    /// all session members including this seat; host is always peer 0
    ostd::vector<session_member> members;
    uint16_t local_peer = 0;
    uint16_t next_peer = 1;
    natural_t listener_id = 0;
    natural_t host_link = 0;

    /// host: link-up instants awaiting JOIN_REQUEST; client: WELCOME deadline
    ostd::map<natural_t, microseconds> pending_joins;
    microseconds join_deadline{ 0 };
    microseconds session_now{ 0 };
    uint64_t tick_count = 0;

    join_validator validator;
    session_observer observer;
    game_event_handler on_game_event;
    ostd::map<uint16_t, frame_handler> handlers;

    session_member* member_by_peer(uint16_t peer_id);
    session_member* member_by_link(natural_t link_id);
    session_member& add_member(node_id node, uint16_t peer_id, natural_t link_id, std::string name);
    void remove_member(uint16_t peer_id);
    void end_session(uint16_t reason);
    void notify(session_event ev, uint16_t arg);

    bool send_session(node_id dst, net_message id, std::span<const uint8_t> payload);
    void send_reject(natural_t link_id, net_reject_reason reason);

    void host_handle_join_request(const link_record& via, std::span<const uint8_t> payload);
    void host_handle_notice(const link_record& via, std::span<const uint8_t> payload);
    void client_handle_welcome(const link_record& via, std::span<const uint8_t> payload);
    void client_handle_reject(std::span<const uint8_t> payload);
    void handle_game_event(const link_record& via, std::span<const uint8_t> payload);
    void adopt_roster_entry(const net_roster_entry& entry, natural_t link_id);
  };

  /// the by-name seam `networking.session-host` resolves through: plugin-provided
  ///  actors (environment registry) park here until the driver mesh spawns one.
  ///  the default name never lands here — the glue builds network_session itself
  class session_actor_source {
   public:
    struct taken {
      natural_t provider_id = 0;
      scope<peer_mesh_actor> actor;
    };

    natural_t provide(scope<peer_mesh_actor> actor);
    void revoke(natural_t id);
    /// null actor = unknown name
    taken take(std::string_view actor_name);

   private:
    ostd::vector<std::pair<natural_t, scope<peer_mesh_actor>>> pending;
    natural_t next_id = 1;
  };

}  // namespace other

#endif  // OTHERLIB_NETWORK_SESSION_NETWORK_SESSION_HPP

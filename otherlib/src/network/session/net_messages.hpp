/**
 * \file network/session/net_messages.hpp
 **/
#ifndef OTHERLIB_NETWORK_SESSION_NET_MESSAGES_HPP
#define OTHERLIB_NETWORK_SESSION_NET_MESSAGES_HPP

#include "core/defines.hpp"

#include "message/message_serialization.hpp"

#include "peer_mesh/mesh_messages.hpp"
#include "peer_mesh/node_id.hpp"

namespace other {

  /// the shipped session actor's message space — one application's use of the
  ///  pass-through net_id tag (D22), imposed on nobody below otherlib
  enum class net_message : uint16_t {
    INVALID = 0,
    JOIN_REQUEST = 1,
    WELCOME,
    REJECT,
    PEER_JOINED,
    PEER_LEFT,
    DISCONNECT_NOTICE,
    /// 05-replication
    JOIN_SNAPSHOT = 32,
    SPAWN,
    DESPAWN,
    TRANSFORM_BATCH,
    COMPONENT_STATE,
    /// 06-world-ops
    SCENE_OP = 64,
    /// gameplay
    GAME_EVENT = 96,
    INPUT_COMMAND,
  };

  /// an application respecting the module's reserved control page
  static_assert(!is_mesh_control(static_cast<uint16_t>(net_message::INPUT_COMMAND)),
                "session message ids must stay below the mesh control page");

  enum class net_reject_reason : uint16_t {
    NONE = 0,
    FULL,
    SHUTTING_DOWN,
    WRONG_TOPOLOGY,
    UNRELIABLE_LINK,
    VALIDATOR,
  };

  /// magic/version/app_hash were already validated at LINK_HELLO — a join can only
  ///  fail on policy. name rides as utf8 bytes (the codec's blob type)
  struct net_join_request {
    uint32_t client_flags = 0;
    ostd::vector<uint8_t> name;
  };

  struct net_roster_entry {
    node_id node = 0;
    uint16_t peer_id = 0;
    ostd::vector<uint8_t> name;
  };

  /// roster = roster_count reflected net_roster_entry records back to back — the
  ///  codec has no vector-of-struct field, so the count carries the shape
  struct net_welcome {
    uint16_t peer_id = 0;
    uint64_t host_tick = 0;
    uint16_t roster_count = 0;
    ostd::vector<uint8_t> roster;
  };

  struct net_reject {
    uint16_t reason = 0;
  };

  struct net_peer_joined {
    net_roster_entry peer;
  };

  /// also the DISCONNECT_NOTICE payload: a member announces its own leave
  ///  (its peer_id), the host announces shutdown (peer_id 0)
  struct net_peer_left {
    uint16_t peer_id = 0;
    uint16_t reason = 0;
  };

  /// name travels as utf8 bytes so the receiving side can surface the string;
  ///  sender_peer is rewritten by the host from the sending link (no spoofing)
  struct net_game_event {
    uint16_t sender_peer = 0;
    ostd::vector<uint8_t> name;
    ostd::vector<uint8_t> payload;
  };

}  // namespace other

OTHER_REFLECT(
  other::net_join_request,
  OTHER_MSG_FIELD(client_flags, CLIENT_FLAGS),
  OTHER_MSG_FIELD(name, DATA))

OTHER_REFLECT(
  other::net_roster_entry,
  OTHER_MSG_FIELD(node, NODE_ID),
  OTHER_MSG_FIELD(peer_id, PEER_ID),
  OTHER_MSG_FIELD(name, DATA))

OTHER_REFLECT(
  other::net_welcome,
  OTHER_MSG_FIELD(peer_id, PEER_ID),
  OTHER_MSG_FIELD(host_tick, HOST_TICK),
  OTHER_MSG_FIELD(roster_count, ROSTER_COUNT),
  OTHER_MSG_FIELD(roster, DATA))

OTHER_REFLECT(
  other::net_reject,
  OTHER_MSG_FIELD(reason, REASON))

OTHER_REFLECT(
  other::net_peer_joined,
  OTHER_MSG_FIELD(peer, DATA))

OTHER_REFLECT(
  other::net_peer_left,
  OTHER_MSG_FIELD(peer_id, PEER_ID),
  OTHER_MSG_FIELD(reason, REASON))

OTHER_REFLECT(
  other::net_game_event,
  OTHER_MSG_FIELD(sender_peer, SENDER_PEER),
  OTHER_MSG_FIELD(name, DATA),
  OTHER_MSG_FIELD(payload, DATA))

#endif  // OTHERLIB_NETWORK_SESSION_NET_MESSAGES_HPP

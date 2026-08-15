/**
 * \file network/mesh_messages.hpp
 **/
#ifndef OTHER_NETWORK_NETWORK_MESH_MESSAGES_HPP
#define OTHER_NETWORK_NETWORK_MESH_MESSAGES_HPP

#include "core/defines.hpp"

#include "network/node_id.hpp"

#include "message/message_serialization.hpp"


namespace other {

  enum class mesh_message : uint16_t {
    LINK_HELLO = 0xFF01,
    LINK_PING = 0xFF02,
    LINK_PONG = 0xFF03,
    LINK_BYE = 0xFF04,
    LINK_AUTH = 0xFF05,
  };

  constexpr static uint16_t kMeshControlFloor = 0xFF00;
  constexpr static uint32_t kMeshMagic = 0x48534D4F;  // 'OMSH'
  constexpr static uint16_t kMeshProtocolVersion = 1;

  inline constexpr bool is_mesh_control(uint16_t net_id) {
    return net_id >= kMeshControlFloor;
  }

  struct mesh_link_hello {
    uint32_t magic = kMeshMagic;
    uint16_t protocol = kMeshProtocolVersion;
    node_id node = 0;
    natural_t app_hash = 0;
    uint8_t reliable = 1;
    uint8_t ordered = 1;
    uint32_t max_frame_size = 0;
  };

  struct mesh_link_ping {
    uint64_t t_send_us = 0;
  };

  struct mesh_link_pong {
    uint64_t t_echo_us = 0;
  };

  struct mesh_link_bye {
    uint16_t reason = 0;
  };

}  // namespace other

OTHER_REFLECT(
  other::mesh_link_hello,
  OTHER_MSG_FIELD(magic, MAGIC),
  OTHER_MSG_FIELD(protocol, PROTOCOL_NUM),
  OTHER_MSG_FIELD(node, NODE_ID),
  OTHER_MSG_FIELD(app_hash, APP_HASH),
  OTHER_MSG_FIELD(reliable, RELIABLE),
  OTHER_MSG_FIELD(ordered, ORDERED),
  OTHER_MSG_FIELD(max_frame_size, MAX_FRAME_SIZE))

OTHER_REFLECT(
  other::mesh_link_ping,
  OTHER_MSG_FIELD(t_send_us, TIMESTAMP_US))

OTHER_REFLECT(
  other::mesh_link_pong,
  OTHER_MSG_FIELD(t_echo_us, TIMESTAMP_US))

OTHER_REFLECT(
  other::mesh_link_bye,
  OTHER_MSG_FIELD(reason, REASON))

#endif  // OTHER_NETWORK_NETWORK_MESH_MESSAGES_HPP

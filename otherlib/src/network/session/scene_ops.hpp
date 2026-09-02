/**
 * \file network/session/scene_ops.hpp
 **/
#ifndef OTHERLIB_NETWORK_SESSION_SCENE_OPS_HPP
#define OTHERLIB_NETWORK_SESSION_SCENE_OPS_HPP

#include <functional>

#include "core/defines.hpp"

#include "message/message_serialization.hpp"

namespace other {

  /// semantic/audit layer: M4 replicates state, this replicates INTENT. the engine
  ///  carries/journals envelopes but never interprets them (gameplay owns that)
  struct scene_op {
    natural_t op_id = 0;    // host-assigned, monotonic, never recycled (spacesim journal rule)
    uint64_t tick = 0;      // host tick at application
    uint16_t actor = 0;     // requesting peer (0 = host)
    natural_t subject = 0;  // primary net_id target, 0 if n/a
    /// op name as utf8 bytes ("construct.place-part") — named C# callbacks need the
    ///  string back, hashes don't reverse; FNV it when an id is wanted
    ostd::vector<uint8_t> name;
    ostd::vector<uint8_t> payload;  // opaque to the engine; gameplay defines meaning
  };

  enum class scene_op_flavor : uint8_t {
    OP_REQUEST = 0,   // client -> host: op_id/tick zero, unvalidated
    OP_APPLIED = 1,   // host -> all: authoritative record
    OP_REJECTED = 2,  // host -> requester only
  };

  /// all flavors share net_message::SCENE_OP; reason is meaningful on REJECTED only
  struct net_scene_op {
    uint8_t flavor = 0;
    uint16_t reason = 0;
    scene_op op;
  };

  struct op_result {
    bool accepted = true;
    uint16_t reason = 0;
  };

}  // namespace other

OTHER_REFLECT(
  other::scene_op,
  OTHER_MSG_FIELD(op_id, OP_ID),
  OTHER_MSG_FIELD(tick, HOST_TICK),
  OTHER_MSG_FIELD(actor, PEER_ID),
  OTHER_MSG_FIELD(subject, NET_ID),
  OTHER_MSG_FIELD(name, DATA),
  OTHER_MSG_FIELD(payload, DATA))

OTHER_REFLECT(
  other::net_scene_op,
  OTHER_MSG_FIELD(flavor, OP_FLAVOR),
  OTHER_MSG_FIELD(reason, REASON),
  OTHER_MSG_FIELD(op, DATA))

#endif  // OTHERLIB_NETWORK_SESSION_SCENE_OPS_HPP

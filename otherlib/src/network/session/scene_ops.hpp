/**
 * \file network/session/scene_ops.hpp
 **/
#ifndef OTHERLIB_NETWORK_SESSION_SCENE_OPS_HPP
#define OTHERLIB_NETWORK_SESSION_SCENE_OPS_HPP

#include <functional>

#include "core/defines.hpp"

#include "network/session/network_session.hpp"

namespace other {

  /// semantic/audit layer: M4 replicates state, this replicates INTENT. the engine
  ///  carries/journals envelopes but never interprets them (gameplay owns that)
  struct scene_op {
    natural_t op_id = 0;  // host-assigned, monotonic, never recycled (spacesim journal rule)
    uint64_t tick = 0;    // host tick at application
    uint16_t actor = 0;   // requesting peer (0 = host)
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

  /// the op pipeline on the default session. glue-owned like replication: it
  ///  registers the SCENE_OP handler itself, the owner forwards session events
  class op_channel {
   public:
    /// gameplay decides — validation and application are one host-side concern;
    ///  a null validator accepts everything (the envelope is audit either way)
    using validator = std::function<op_result(uint16_t peer, const scene_op& op)>;
    using applied_handler = std::function<void(const scene_op& op)>;
    using rejected_handler = std::function<void(std::string_view name, uint16_t reason)>;

    op_channel(network_session& session, std::function<uint64_t()> tick_source, size_t journal_cap = 4096);

    void on_session_event(session_event ev, uint16_t arg);

    /// client seat: sends OP_REQUEST to the host. host seat: the same write path
    ///  minus the request hop — validator skipped, actor 0
    bool request(std::string_view name, natural_t subject, std::span<const uint8_t> payload);

    void set_validator(validator fn) { validate = std::move(fn); }
    void set_applied_handler(applied_handler fn) { on_applied = std::move(fn); }
    void set_rejected_handler(rejected_handler fn) { on_rejected = std::move(fn); }

    /// session-scoped audit, host-side, ordered by op_id (a ring: oldest evicted)
    std::span<const scene_op> journal() const { return entries; }

   private:
    network_session& session;
    std::function<uint64_t()> tick_source;
    size_t journal_cap;

    natural_t next_op_id = 1;
    ostd::vector<scene_op> entries;

    validator validate;
    applied_handler on_applied;
    rejected_handler on_rejected;

    void handle_scene_op(node_id src, std::span<const uint8_t> payload);
    /// the one write path: assign id/tick, journal, broadcast APPLIED, dispatch
    void host_apply(scene_op&& op);
    void send_rejection(uint16_t peer, const scene_op& op, uint16_t reason);
    uint16_t peer_of(node_id node) const;
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

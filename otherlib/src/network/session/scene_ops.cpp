/**
 * \file network/session/scene_ops.cpp
 **/
#include "network/session/scene_ops.hpp"

#include <algorithm>

#include "core/logger.hpp"
#include "core/profiler.hpp"

namespace other {

  namespace {

    std::string_view name_view(const ostd::vector<uint8_t>& bytes) {
      return { reinterpret_cast<const char*>(bytes.data()), bytes.size() };
    }

  }  // namespace

  op_channel::op_channel(network_session& session, std::function<uint64_t()> tick_source, size_t journal_cap)
      : session(session), tick_source(std::move(tick_source)), journal_cap(journal_cap) {
    session.register_handler(net_message::SCENE_OP,
                             [this](node_id src, std::span<const uint8_t> payload) { handle_scene_op(src, payload); });
  }

  void op_channel::on_session_event(session_event ev, uint16_t arg) {
    if (ev == session_event::STARTED) {
      /// session-scoped: a fresh session starts a fresh journal; the previous one
      ///  stays readable between sessions for post-mortems
      entries.clear();
      next_op_id = 1;
    }
  }

  bool op_channel::request(std::string_view name, natural_t subject, std::span<const uint8_t> payload) {
    if (!session.in_session()) {
      CORE_LOG_WARN("[OPS] request refused: not in a session");
      return false;
    }

    scene_op op;
    op.subject = subject;
    op.name = ostd::vector<uint8_t>(name.begin(), name.end());
    op.payload = ostd::vector<uint8_t>(payload.begin(), payload.end());

    if (session.is_host()) {
      /// host-initiated: the same write path minus the request hop
      op.actor = 0;
      host_apply(std::move(op));
      return true;
    }

    const net_scene_op wire{ .flavor = static_cast<uint8_t>(scene_op_flavor::OP_REQUEST), .op = std::move(op) };
    return session.send(0, net_message::SCENE_OP, serialize_direct(wire));
  }

  void op_channel::handle_scene_op(node_id src, std::span<const uint8_t> payload) {
    PROFILE_SECTION("op_channel::handle_scene_op");
    net_scene_op wire;
    try {
      wire = deserialize_direct<net_scene_op>(payload).first;
    } catch (const std::exception&) {
      CORE_LOG_WARN("[OPS] malformed SCENE_OP dropped");
      return;
    }

    switch (static_cast<scene_op_flavor>(wire.flavor)) {
      case scene_op_flavor::OP_REQUEST: {
        if (!session.is_host()) {
          return;
        }
        const uint16_t peer = peer_of(src);
        if (peer == 0) {
          return;  // not a member seat
        }
        wire.op.actor = peer;
        const op_result verdict = validate != nullptr ? validate(peer, wire.op) : op_result{};
        if (!verdict.accepted) {
          send_rejection(peer, wire.op, verdict.reason);
          return;
        }
        host_apply(std::move(wire.op));
        return;
      }

      case scene_op_flavor::OP_APPLIED:
        /// presentation/bookkeeping only on clients — state arrives via M4
        if (!session.is_host() && on_applied != nullptr) {
          on_applied(wire.op);
        }
        return;

      case scene_op_flavor::OP_REJECTED:
        if (!session.is_host() && on_rejected != nullptr) {
          on_rejected(name_view(wire.op.name), wire.reason);
        }
        return;
    }
    CORE_LOG_WARN("[OPS] unknown scene op flavor {}", wire.flavor);
  }

  void op_channel::host_apply(scene_op&& op) {
    PROFILE_SECTION("op_channel::host_apply");
    op.op_id = next_op_id++;
    op.tick = tick_source != nullptr ? tick_source() : 0;

    entries.push_back(op);
    if (entries.size() > journal_cap) {
      entries.erase(entries.begin(), entries.begin() + (entries.size() - journal_cap));
    }

    const net_scene_op wire{ .flavor = static_cast<uint8_t>(scene_op_flavor::OP_APPLIED), .op = op };
    session.broadcast(net_message::SCENE_OP, serialize_direct(wire));

    /// the host runs its own applied handler too — one dispatch path for everyone
    if (on_applied != nullptr) {
      on_applied(op);
    }
  }

  void op_channel::send_rejection(uint16_t peer, const scene_op& op, uint16_t reason) {
    net_scene_op wire{ .flavor = static_cast<uint8_t>(scene_op_flavor::OP_REJECTED), .reason = reason };
    wire.op.name = op.name;
    session.send(peer, net_message::SCENE_OP, serialize_direct(wire));
  }

  uint16_t op_channel::peer_of(node_id node) const {
    auto itr = std::ranges::find(session.peers(), node, &session_member::node);
    return itr != session.peers().end() ? itr->peer_id : 0;
  }

}  // namespace other

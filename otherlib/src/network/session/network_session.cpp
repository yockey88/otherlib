/**
 * \file network/session/network_session.cpp
 **/
#include "network/session/network_session.hpp"

#include <algorithm>

#include "core/logger.hpp"
#include "core/profiler.hpp"

#include "peer_mesh/peer_mesh.hpp"

namespace other {

  namespace {

    ostd::vector<uint8_t> name_bytes(std::string_view text) {
      return ostd::vector<uint8_t>(text.begin(), text.end());
    }

    std::string name_string(std::span<const uint8_t> bytes) {
      return std::string(bytes.begin(), bytes.end());
    }

  }  // namespace

  void network_session::set_role(session_role next) {
    /// role transitions are load-bearing session moments; engine events ride the
    ///  session observer, this is the module-level trace
    state = next;
    CORE_LOG_DEBUG("peer role -> {}", static_cast<uint8_t>(next));
  }

  /// ---------------------------------------------------------------- public api

  bool network_session::host(const net_address& bind) {
    PROFILE_SECTION("network_session::host");
    if (!spawned() || state != session_role::UNJOINED) {
      CORE_LOG_WARN("[SESSION] host refused: {}", spawned() ? "already in a session" : "session is not spawned on a mesh");
      return false;
    }

    /// re-hosting reuses the still-open listener; the mesh has no listener close
    ///  (recorded for M5 ops)
    if (listener_id == 0) {
      listener_id = open_listener(bind);
    }
    if (listener_id == 0) {
      CORE_LOG_WARN("[SESSION] host failed: listener refused");
      return false;
    }

    set_role(session_role::SERVER);
    local_peer = 0;
    next_peer = 1;
    add_member(id(), 0, 0, cfg.display_name);
    notify(session_event::STARTED, 0);
    return true;
  }

  bool network_session::join(const net_address& remote) {
    PROFILE_SECTION("network_session::join");
    if (!spawned() || state != session_role::UNJOINED) {
      CORE_LOG_WARN("[SESSION] join refused: {}", spawned() ? "already in a session" : "session is not spawned on a mesh");
      return false;
    }

    host_link = open_link(remote);
    if (host_link == 0) {
      CORE_LOG_WARN("[SESSION] join failed: dial refused");
      return false;
    }

    set_role(session_role::JOINING);
    join_deadline = session_now + cfg.join_timeout;
    return true;
  }

  void network_session::leave(uint16_t reason) {
    PROFILE_SECTION("network_session::leave");
    if (!spawned() || state == session_role::UNJOINED) {
      return;
    }

    const net_peer_left notice{ .peer_id = local_peer, .reason = reason };
    const ostd::vector<uint8_t> bytes = serialize_direct(notice);
    if (is_host()) {
      for (const session_member& member : members) {
        if (member.link_id != 0) {
          send_on_link(member.link_id, static_cast<uint16_t>(net_message::DISCONNECT_NOTICE), bytes);
        }
      }
    } else if (state == session_role::PEER) {
      send_on_link(host_link, static_cast<uint16_t>(net_message::DISCONNECT_NOTICE), bytes);
    }

    end_session(reason);
  }

  bool network_session::send(uint16_t peer_id, net_message msg_id, std::span<const uint8_t> payload) {
    if (!in_session()) {
      CORE_LOG_WARN("[SESSION] send refused: not in a session");
      return false;
    }
    session_member* member = member_by_peer(peer_id);
    if (member == nullptr || member->node == id()) {
      return false;
    }
    /// always single-hop under star, but written against the mesh API — a relayed
    ///  topology would not change this call site
    return peer_actor::send(member->node, static_cast<uint16_t>(msg_id), payload);
  }

  bool network_session::broadcast(net_message msg_id, std::span<const uint8_t> payload) {
    if (!is_host()) {
      CORE_LOG_WARN("[SESSION] broadcast refused: not hosting");
      return false;
    }
    bool all_sent = true;
    for (const session_member& member : members) {
      if (member.node != id()) {
        all_sent &= peer_actor::send(member.node, static_cast<uint16_t>(msg_id), payload);
      }
    }
    return all_sent;
  }

  bool network_session::send_game_event(std::string_view event_name, std::span<const uint8_t> payload) {
    if (!in_session() || is_host()) {
      CORE_LOG_WARN("[SESSION] send_game_event refused: requires a joined client seat");
      return false;
    }
    const net_game_event ev{
      .sender_peer = local_peer,
      .name = name_bytes(event_name),
      .payload = ostd::vector<uint8_t>(payload.begin(), payload.end()),
    };
    return send(0, net_message::GAME_EVENT, serialize_direct(ev));
  }

  bool network_session::broadcast_game_event(std::string_view event_name, std::span<const uint8_t> payload) {
    if (!is_host()) {
      CORE_LOG_WARN("[SESSION] broadcast_game_event refused: not hosting");
      return false;
    }
    const net_game_event ev{
      .sender_peer = 0,
      .name = name_bytes(event_name),
      .payload = ostd::vector<uint8_t>(payload.begin(), payload.end()),
    };
    return broadcast(net_message::GAME_EVENT, serialize_direct(ev));
  }

  void network_session::register_handler(net_message msg_id, frame_handler fn) {
    handlers[static_cast<uint16_t>(msg_id)] = std::move(fn);
  }

  /// ---------------------------------------------------------------- actor hooks

  void network_session::on_link_up(const link_record& link) {
    if (state == session_role::JOINING && link.link_id == host_link) {
      const net_join_request request{ .client_flags = cfg.client_flags, .name = name_bytes(cfg.display_name) };
      send_on_link(host_link, static_cast<uint16_t>(net_message::JOIN_REQUEST), serialize_direct(request));
      join_deadline = session_now + cfg.join_timeout;
      return;
    }
    if (is_host()) {
      pending_joins[link.link_id] = session_now;
    }
  }

  void network_session::on_link_down(const link_record& link, link_close_reason reason) {
    if (is_host()) {
      pending_joins.erase(link.link_id);
      if (session_member* member = member_by_link(link.link_id); member != nullptr) {
        /// liveness is link state: this IS the leave signal
        const uint16_t peer = member->peer_id;
        remove_member(peer);
        const net_peer_left left{ .peer_id = peer, .reason = static_cast<uint16_t>(reason) };
        broadcast(net_message::PEER_LEFT, serialize_direct(left));
        notify(session_event::PEER_LEFT, peer);
      }
      return;
    }
    if (link.link_id == host_link && state != session_role::UNJOINED) {
      /// link-down to the host = session over
      end_session(static_cast<uint16_t>(reason));
    }
  }

  void network_session::tick(microseconds now, double dt) {
    session_now = now;
    tick_count++;

    if (is_host()) {
      ostd::vector<natural_t> expired;
      for (const auto& [link_id, upped_at] : pending_joins) {
        if (now - upped_at > cfg.join_timeout) {
          expired.push_back(link_id);
        }
      }
      for (const natural_t link_id : expired) {
        CORE_LOG_WARN("[SESSION] dropping silent link {} (no JOIN_REQUEST)", link_id);
        pending_joins.erase(link_id);
        close_link(link_id, link_close_reason::HANDSHAKE_TIMEOUT);
      }
      return;
    }

    if (state == session_role::JOINING && now > join_deadline) {
      CORE_LOG_WARN("[SESSION] join timed out waiting for WELCOME");
      close_link(host_link, link_close_reason::HANDSHAKE_TIMEOUT);
    }
  }

  void network_session::on_frame(const link_record& via, node_id src, uint16_t net_id, std::span<const uint8_t> payload) {
    PROFILE_SECTION("network_session::on_frame");
    const net_message msg_id = static_cast<net_message>(net_id);

    /// client rule: session frames before WELCOME/REJECT close the link
    if (state == session_role::JOINING &&
        msg_id != net_message::WELCOME && msg_id != net_message::REJECT) {
      CORE_LOG_WARN("[SESSION] session frame {} before WELCOME; closing link", net_id);
      close_link(via.link_id, link_close_reason::PROTOCOL_ERROR);
      return;
    }

    switch (msg_id) {
      case net_message::JOIN_REQUEST:
        if (is_host()) {
          host_handle_join_request(via, payload);
        } else {
          /// peers never link to each other in v1 — this actor's topology, asserted here
          send_reject(via.link_id, net_reject_reason::WRONG_TOPOLOGY);
          close_link(via.link_id, link_close_reason::SHUTDOWN);
        }
        return;

      case net_message::WELCOME:
        if (state == session_role::JOINING && via.link_id == host_link) {
          client_handle_welcome(via, payload);
        }
        return;

      case net_message::REJECT:
        if (state == session_role::JOINING) {
          client_handle_reject(payload);
        }
        return;

      case net_message::PEER_JOINED:
        if (state == session_role::PEER && via.link_id == host_link) {
          try {
            const net_peer_joined joined = deserialize_direct<net_peer_joined>(payload).first;
            adopt_roster_entry(joined.peer, 0);
            notify(session_event::PEER_JOINED, joined.peer.peer_id);
          } catch (const std::exception&) {
            CORE_LOG_WARN("[SESSION] malformed PEER_JOINED dropped");
          }
        }
        return;

      case net_message::PEER_LEFT:
        if (state == session_role::PEER && via.link_id == host_link) {
          try {
            const net_peer_left left = deserialize_direct<net_peer_left>(payload).first;
            remove_member(left.peer_id);
            notify(session_event::PEER_LEFT, left.peer_id);
          } catch (const std::exception&) {
            CORE_LOG_WARN("[SESSION] malformed PEER_LEFT dropped");
          }
        }
        return;

      case net_message::DISCONNECT_NOTICE:
        if (is_host()) {
          host_handle_notice(via, payload);
        } else if (state == session_role::PEER && via.link_id == host_link) {
          try {
            end_session(deserialize_direct<net_peer_left>(payload).first.reason);
          } catch (const std::exception&) {
            end_session(0);
          }
        }
        return;

      case net_message::GAME_EVENT:
        handle_game_event(via, payload);
        return;

      default:
        break;
    }

    if (auto itr = handlers.find(net_id); itr != handlers.end() && in_session()) {
      itr->second(src, payload);
      return;
    }
    CORE_LOG_TRACE("[SESSION] unhandled session frame {} from node {}", net_id, src);
  }

  /// ---------------------------------------------------------------- host side

  void network_session::host_handle_join_request(const link_record& via, std::span<const uint8_t> payload) {
    PROFILE_SECTION("network_session::host_handle_join_request");
    pending_joins.erase(via.link_id);

    net_join_request request;
    try {
      request = deserialize_direct<net_join_request>(payload).first;
    } catch (const std::exception&) {
      close_link(via.link_id, link_close_reason::PROTOCOL_ERROR);
      return;
    }

    /// joins can only fail on policy — protocol/app compatibility was proven at
    ///  LINK_HELLO. datagram links are Mode-4 land; sessions refuse them (D11)
    net_reject_reason refusal = net_reject_reason::NONE;
    if (!via.caps.reliable || !via.caps.ordered) {
      refusal = net_reject_reason::UNRELIABLE_LINK;
    } else if (members.size() >= cfg.max_peers) {
      refusal = net_reject_reason::FULL;
    } else if (std::ranges::find(members, via.remote, &session_member::node) != members.end()) {
      refusal = net_reject_reason::WRONG_TOPOLOGY;
    } else if (validator != nullptr) {
      if (const opt<uint16_t> reason = validator(via, request); reason.has_value()) {
        send_reject(via.link_id, static_cast<net_reject_reason>(*reason));
        close_link(via.link_id, link_close_reason::SHUTDOWN);
        return;
      }
    }
    if (refusal != net_reject_reason::NONE) {
      send_reject(via.link_id, refusal);
      close_link(via.link_id, link_close_reason::SHUTDOWN);
      return;
    }

    while (next_peer == 0 || member_by_peer(next_peer) != nullptr) {
      next_peer++;
    }
    const uint16_t peer = next_peer++;
    add_member(via.remote, peer, via.link_id, name_string(request.name));

    net_welcome welcome{ .peer_id = peer, .host_tick = tick_count, .roster_count = static_cast<uint16_t>(members.size()) };
    for (const session_member& member : members) {
      const net_roster_entry entry{ .node = member.node, .peer_id = member.peer_id, .name = name_bytes(member.name) };
      welcome.roster.append_range(serialize_direct(entry));
    }
    send_on_link(via.link_id, static_cast<uint16_t>(net_message::WELCOME), serialize_direct(welcome));

    const net_peer_joined joined{ .peer = { .node = via.remote, .peer_id = peer, .name = request.name } };
    const ostd::vector<uint8_t> joined_bytes = serialize_direct(joined);
    for (const session_member& member : members) {
      if (member.node != id() && member.peer_id != peer) {
        peer_actor::send(member.node, static_cast<uint16_t>(net_message::PEER_JOINED), joined_bytes);
      }
    }
    notify(session_event::PEER_JOINED, peer);
  }

  void network_session::host_handle_notice(const link_record& via, std::span<const uint8_t> payload) {
    session_member* member = member_by_link(via.link_id);
    if (member == nullptr) {
      return;
    }
    uint16_t reason = 0;
    try {
      reason = deserialize_direct<net_peer_left>(payload).first.reason;
    } catch (const std::exception&) {
    }

    const uint16_t peer = member->peer_id;
    remove_member(peer);
    close_link(via.link_id, link_close_reason::SHUTDOWN);
    const net_peer_left left{ .peer_id = peer, .reason = reason };
    broadcast(net_message::PEER_LEFT, serialize_direct(left));
    notify(session_event::PEER_LEFT, peer);
  }

  /// ---------------------------------------------------------------- client side

  void network_session::client_handle_welcome(const link_record& via, std::span<const uint8_t> payload) {
    PROFILE_SECTION("network_session::client_handle_welcome");
    try {
      const net_welcome welcome = deserialize_direct<net_welcome>(payload).first;
      local_peer = welcome.peer_id;
      std::span<const uint8_t> rest{ welcome.roster };
      for (uint16_t i = 0; i < welcome.roster_count; ++i) {
        auto [entry, used] = deserialize_direct<net_roster_entry>(rest);
        adopt_roster_entry(entry, entry.peer_id == 0 ? host_link : 0);
        rest = rest.subspan(used);
      }
    } catch (const std::exception& error) {
      CORE_LOG_WARN("[SESSION] malformed WELCOME: {}", error.what());
      close_link(via.link_id, link_close_reason::PROTOCOL_ERROR);
      return;
    }

    set_role(session_role::PEER);
    notify(session_event::STARTED, local_peer);
  }

  void network_session::client_handle_reject(std::span<const uint8_t> payload) {
    uint16_t reason = static_cast<uint16_t>(net_reject_reason::NONE);
    try {
      reason = deserialize_direct<net_reject>(payload).first.reason;
    } catch (const std::exception&) {
    }
    CORE_LOG_WARN("[SESSION] join rejected (reason {})", reason);
    end_session(reason);
  }

  /// ---------------------------------------------------------------- shared

  void network_session::handle_game_event(const link_record& via, std::span<const uint8_t> payload) {
    net_game_event ev;
    try {
      ev = deserialize_direct<net_game_event>(payload).first;
    } catch (const std::exception&) {
      CORE_LOG_WARN("[SESSION] malformed GAME_EVENT dropped");
      return;
    }

    if (is_host()) {
      session_member* member = member_by_link(via.link_id);
      if (member == nullptr) {
        return;
      }
      ev.sender_peer = member->peer_id;  // the sending link names the sender
    } else if (!in_session() || via.link_id != host_link) {
      return;
    }

    if (on_game_event != nullptr) {
      const std::string event_name = name_string(ev.name);
      on_game_event(ev.sender_peer, event_name, ev.payload);
    }
  }

  void network_session::adopt_roster_entry(const net_roster_entry& entry, natural_t link_id) {
    if (std::ranges::find(members, entry.node, &session_member::node) != members.end()) {
      return;
    }
    add_member(entry.node, entry.peer_id, link_id, name_string(entry.name));
  }

  session_member* network_session::member_by_peer(uint16_t peer_id) {
    auto itr = std::ranges::find(members, peer_id, &session_member::peer_id);
    return itr != members.end() ? &*itr : nullptr;
  }

  session_member* network_session::member_by_link(natural_t link_id) {
    if (link_id == 0) {
      return nullptr;
    }
    auto itr = std::ranges::find(members, link_id, &session_member::link_id);
    return itr != members.end() ? &*itr : nullptr;
  }

  session_member& network_session::add_member(node_id node, uint16_t peer_id, natural_t link_id, std::string member_name) {
    members.push_back({ .node = node, .peer_id = peer_id, .link_id = link_id, .name = std::move(member_name) });
    return members.back();
  }

  void network_session::remove_member(uint16_t peer_id) {
    auto itr = std::ranges::find(members, peer_id, &session_member::peer_id);
    if (itr == members.end()) {
      return;
    }
    members.erase(itr);
  }

  void network_session::end_session(uint16_t reason) {
    PROFILE_SECTION("network_session::end_session");
    ostd::vector<natural_t> links_to_close;
    for (const session_member& member : members) {
      if (member.link_id != 0) {
        links_to_close.push_back(member.link_id);
      }
    }
    members.clear();
    pending_joins.clear();
    host_link = 0;
    local_peer = 0;

    /// links close after membership is gone so re-entrant on_link_down finds no member
    for (const natural_t link_id : links_to_close) {
      close_link(link_id, link_close_reason::SHUTDOWN);
    }

    if (state != session_role::UNJOINED) {
      set_role(session_role::UNJOINED);
    }
    notify(session_event::ENDED, reason);
  }

  void network_session::notify(session_event ev, uint16_t arg) {
    if (observer != nullptr) {
      observer(ev, arg);
    }
  }

  void network_session::send_reject(natural_t link_id, net_reject_reason reason) {
    const net_reject reject{ .reason = static_cast<uint16_t>(reason) };
    send_on_link(link_id, static_cast<uint16_t>(net_message::REJECT), serialize_direct(reject));
  }

  /// ---------------------------------------------------------------- actor source

  natural_t session_actor_source::provide(scope<peer_actor> actor) {
    OTHER_ASSERT(actor != nullptr, "Cannot provide a null session actor.");
    CORE_LOG_DEBUG("[SESSION] actor '{}' provided by name", actor->name());
    pending.emplace_back(next_id, std::move(actor));
    return next_id++;
  }

  void session_actor_source::revoke(natural_t id) {
    std::erase_if(pending, [id](const auto& entry) { return entry.first == id; });
  }

  session_actor_source::taken session_actor_source::take(std::string_view actor_name) {
    auto itr = std::ranges::find_if(pending, [&](const auto& entry) { return entry.second->name() == actor_name; });
    if (itr == pending.end()) {
      CORE_LOG_ERROR("[SESSION] no session actor named '{}' has been provided", actor_name);
      return {};
    }
    taken result{ itr->first, std::move(itr->second) };
    pending.erase(itr);
    return result;
  }

}  // namespace other

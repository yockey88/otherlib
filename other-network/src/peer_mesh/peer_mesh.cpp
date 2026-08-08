/**
 * \file peer_mesh/peer_mesh.cpp
 **/
#include "peer_mesh/peer_mesh.hpp"

#include <algorithm>

#include "core/logger.hpp"
#include "core/profiler.hpp"

#include "message/message_serialization.hpp"

#include "peer_mesh/mesh_messages.hpp"

namespace other {

  namespace {

    inline uint16_t control_id(mesh_message id) {
      return static_cast<uint16_t>(id);
    }

  }  // namespace

  peer_mesh::peer_mesh(std::string_view debug_name, const peer_mesh_config& config)
      : debug_name(debug_name), cfg(config), active_router(make_scope<direct_router>()) {}

  peer_mesh::~peer_mesh() {
    /// close every live link; transports outlive the mesh (not owned)
    ostd::vector<natural_t> live;
    for (const link_record& record : links.links()) {
      live.push_back(record.link_id);
    }
    for (const natural_t id : live) {
      teardown(id, link_close_reason::SHUTDOWN, true);
    }
  }

  void peer_mesh::register_transport(link_transport& transport) {
    for (const transport_entry& entry : transports) {
      OTHER_ASSERT(entry.transport != &transport, "Transport '{}' is already registered with mesh '{}'.", transport.name(), debug_name);
      OTHER_ASSERT(entry.transport->name() != transport.name(), "A transport named '{}' is already registered with mesh '{}'.", transport.name(), debug_name);
    }

    const size_t index = transports.size();
    transports.push_back({ &transport });

    transport.bind({
      .opened = [this, index](natural_t conn_id) { on_conn_opened(index, conn_id); },
      .accepted = [this, index](natural_t listener_id, natural_t conn_id) { on_conn_accepted(index, listener_id, conn_id); },
      .received = [this, index](natural_t conn_id, std::span<const uint8_t> bytes) { on_conn_received(index, conn_id, bytes); },
      .closed = [this, index](natural_t conn_id) { on_conn_closed(index, conn_id); },
    });
  }

  link_transport* peer_mesh::transport(std::string_view transport_name) {
    for (const transport_entry& entry : transports) {
      if (entry.transport->name() == transport_name) {
        return entry.transport;
      }
    }
    return nullptr;
  }

  peer_mesh_actor& peer_mesh::spawn_actor(scope<peer_mesh_actor> actor, node_id id) {
    OTHER_ASSERT(actor != nullptr, "Cannot spawn a null actor.");
    OTHER_ASSERT(id != 0, "Actor node id 0 is reserved for invalid/unknown.");
    OTHER_ASSERT(this->actor(id) == nullptr, "An actor with node id {} is already resident in mesh '{}'.", id, debug_name);

    actor->owner = this;
    actor->node = id;

    peer_record& record = nodes.ensure_node(id);
    record.resident = true;
    record.last_seen_tick = tick_counter;

    resident_actors.push_back(std::move(actor));
    return *resident_actors.back();
  }

  void peer_mesh::destroy_actor(node_id id) {
    auto itr = std::ranges::find_if(resident_actors, [id](const scope<peer_mesh_actor>& a) { return a->node == id; });
    if (itr == resident_actors.end()) {
      CORE_LOG_WARN("[MESH {}] destroy_actor: no resident actor with node id {}", debug_name, id);
      return;
    }

    ostd::vector<natural_t> owned;
    for (const link_record& record : links.links()) {
      if (record.local == id) {
        owned.push_back(record.link_id);
      }
    }
    for (const natural_t link_id : owned) {
      teardown(link_id, link_close_reason::ACTOR_DESTROYED, true);
    }

    /// listeners owned by this actor stop accepting into the mesh
    std::erase_if(listener_owners, [id](const auto& entry) { return entry.second == id; });

    nodes.remove_node(id);
    itr = std::ranges::find_if(resident_actors, [id](const scope<peer_mesh_actor>& a) { return a->node == id; });
    if (itr != resident_actors.end()) {
      (*itr)->owner = nullptr;
      resident_actors.erase(itr);
    }
  }

  peer_mesh_actor* peer_mesh::actor(node_id id) {
    auto itr = std::ranges::find_if(resident_actors, [id](const scope<peer_mesh_actor>& a) { return a->node == id; });
    return itr != resident_actors.end() ? itr->get() : nullptr;
  }

  void peer_mesh::add_rx_filter(mesh_filter fn) {
    rx_filters.push_back(std::move(fn));
  }

  void peer_mesh::add_tx_filter(mesh_filter fn) {
    tx_filters.push_back(std::move(fn));
  }

  void peer_mesh::set_router(scope<mesh_router> new_router) {
    OTHER_ASSERT(new_router != nullptr, "Cannot install a null router; direct_router is the default.");
    active_router = std::move(new_router);
    for (const link_record& record : links.links()) {
      if (record.state == link_state::UP) {
        active_router->on_link_up(record);
      }
    }
  }

  mesh_router& peer_mesh::router() {
    return *active_router;
  }

  void peer_mesh::set_security(scope<link_security> new_security) {
    OTHER_ASSERT(links.link_count() == 0, "Security layer must be installed before any link exists.");
    link_sec = std::move(new_security);
  }

  /// ---------------------------------------------------------------- topology ops

  natural_t peer_mesh::resolve_transport(const net_address& remote, std::string_view transport_name, size_t& out_index) {
    std::string_view wanted = transport_name;
    if (wanted.empty()) {
      switch (remote.addressing) {
        case net_address::kind::MEMORY: wanted = "memory"; break;
        case net_address::kind::IP: wanted = "tcp"; break;
        case net_address::kind::STEAM_PEER:
        case net_address::kind::STEAM_LOBBY: wanted = "steam"; break;
      }
    }
    for (size_t i = 0; i < transports.size(); ++i) {
      if (transports[i].transport->name() == wanted) {
        out_index = i;
        return 1;
      }
    }
    CORE_LOG_WARN("[MESH {}] no transport '{}' registered", debug_name, wanted);
    return 0;
  }

  natural_t peer_mesh::open_link_from(peer_mesh_actor& from, const net_address& remote, std::string_view transport_name) {
    PROFILE_SECTION("peer_mesh::open_link");
    if (links.link_count() >= cfg.max_links) {
      CORE_LOG_WARN("[MESH {}] open_link refused: at max links ({})", debug_name, cfg.max_links);
      stats.refused_sends++;
      return 0;
    }

    size_t transport_index = 0;
    if (resolve_transport(remote, transport_name, transport_index) == 0) {
      return 0;
    }

    link_transport& t = *transports[transport_index].transport;
    const natural_t conn_id = t.dial(remote);
    if (conn_id == 0) {
      return 0;
    }

    link_record& record = links.adopt(transport_index, conn_id, from.node, link_state::CONNECTING,
                                      t.conn_caps(conn_id), t.is_stream(), cfg.max_frame_size, current_now);
    CORE_LOG_TRACE("[MESH {}] link {} dialing via '{}' (local node {})", debug_name, record.link_id, t.name(), from.node);
    return record.link_id;
  }

  natural_t peer_mesh::open_listener_from(peer_mesh_actor& from, const net_address& bind, std::string_view transport_name) {
    size_t transport_index = 0;
    if (resolve_transport(bind, transport_name, transport_index) == 0) {
      return 0;
    }

    link_transport& t = *transports[transport_index].transport;
    const natural_t listener_id = t.listen(bind);
    if (listener_id == 0) {
      return 0;
    }

    listener_owners[{ transport_index, listener_id }] = from.node;
    return listener_id;
  }

  ostd::vector<link_record> peer_mesh::links_of(node_id local) const {
    ostd::vector<link_record> out;
    for (const link_record& record : links.links()) {
      if (record.local == local) {
        out.push_back(record);
      }
    }
    return out;
  }

  void peer_mesh::close_link(natural_t link_id, link_close_reason reason) {
    if (links.link(link_id) == nullptr) {
      return;
    }
    teardown(link_id, reason, true);
  }

  /// ---------------------------------------------------------------- transport events

  void peer_mesh::on_conn_opened(size_t transport_index, natural_t conn_id) {
    const natural_t link_id = links.link_for_conn(transport_index, conn_id);
    link_record* record = links.mutable_link(link_id);
    if (record == nullptr) {
      return;
    }
    record->state = link_state::HANDSHAKING;
    send_hello(link_id);
  }

  void peer_mesh::on_conn_accepted(size_t transport_index, natural_t listener_id, natural_t conn_id) {
    auto owner = listener_owners.find({ transport_index, listener_id });
    link_transport& t = *transports[transport_index].transport;
    if (owner == listener_owners.end() || links.link_count() >= cfg.max_links) {
      t.close(conn_id);
      return;
    }

    links.adopt(transport_index, conn_id, owner->second, link_state::HANDSHAKING,
                t.conn_caps(conn_id), t.is_stream(), cfg.max_frame_size, current_now);
    const natural_t link_id = links.link_for_conn(transport_index, conn_id);
    send_hello(link_id);
  }

  void peer_mesh::on_conn_received(size_t transport_index, natural_t conn_id, std::span<const uint8_t> bytes) {
    PROFILE_SECTION("peer_mesh::rx");
    const natural_t link_id = links.link_for_conn(transport_index, conn_id);
    network::link_runtime* rt = links.runtime(link_id);
    if (rt == nullptr) {
      return;
    }

    rt->last_rx = current_now;
    if (link_record* record = links.mutable_link(link_id); record != nullptr) {
      record->latest_rx_tick = tick_counter;
      record->stats.bytes_rx += bytes.size();
    }

    if (rt->stream) {
      rt->reader.feed(bytes);
      while (true) {
        /// re-fetch: frame processing may tear the link down
        rt = links.runtime(link_id);
        if (rt == nullptr) {
          return;
        }
        frame_parse_result result = rt->reader.next();
        if (result.err == frame_parse_result::error::NEED_MORE) {
          return;
        }
        if (result.err != frame_parse_result::error::NONE) {
          stats.malformed_frames++;
          protocol_error(link_id, "malformed or oversize stream frame");
          return;
        }
        process_frame(link_id, std::move(*result.frame));
      }
    }

    frame_parse_result result = parse_frame_exact(bytes, cfg.max_frame_size);
    if (result.err != frame_parse_result::error::NONE) {
      stats.malformed_frames++;
      protocol_error(link_id, "delivery is not exactly one frame");
      return;
    }
    process_frame(link_id, std::move(*result.frame));
  }

  void peer_mesh::on_conn_closed(size_t transport_index, natural_t conn_id) {
    const natural_t link_id = links.link_for_conn(transport_index, conn_id);
    if (links.link(link_id) == nullptr) {
      return;
    }
    teardown(link_id, link_close_reason::TRANSPORT_CLOSED, false);
  }

  /// ---------------------------------------------------------------- rx pipeline

  void peer_mesh::process_frame(natural_t link_id, parsed_frame&& frame) {
    link_record* record = links.mutable_link(link_id);
    if (record == nullptr) {
      return;
    }
    record->stats.frames_rx++;

    if (peer_record* node = nodes.record(record->remote); node != nullptr) {
      node->last_seen_tick = tick_counter;
    }

    if (is_mesh_control(frame.net_id)) {
      handle_control(link_id, frame.net_id, frame.payload);
      return;
    }

    if (record->state != link_state::UP) {
      protocol_error(link_id, "application frame before link up");
      return;
    }

    /// decrypt first — filters and routing see plaintext; control frames never get here
    if (link_sec != nullptr) {
      if (!link_sec->decrypt(*record, std::span<uint8_t>(frame.payload))) {
        stats.security_failures++;
        teardown(link_id, link_close_reason::SECURITY_ERROR, true);
        return;
      }
    }

    node_id src = record->remote;
    node_id dst = record->local;
    uint8_t ttl = 0;
    bool routed = false;
    std::span<const uint8_t> payload{ frame.payload };

    if ((frame.flags & static_cast<uint16_t>(frame_flags::ROUTED)) != 0) {
      opt<route_header> route = read_route_header(payload);
      if (!route.has_value() || route->dst == 0) {
        protocol_error(link_id, "malformed route header");
        return;
      }
      src = route->src;
      dst = route->dst;
      ttl = route->ttl;
      routed = true;
      payload = payload.subspan(kRouteHeaderSize);
    }

    deliver_or_forward(link_id, src, dst, ttl, routed, frame.net_id, payload);
  }

  bool peer_mesh::run_filters(ostd::vector<mesh_filter>& chain, const link_record& link, node_id src, node_id dst,
                              uint16_t net_id, std::span<const uint8_t>& payload, ostd::vector<uint8_t>& scratch) {
    if (chain.empty()) {
      return true;
    }

    mesh_frame_view view{ .link = link, .src = src, .dst = dst, .net_id = net_id, .payload = payload };
    for (mesh_filter& filter : chain) {
      filter(view);
      if (view.dropped) {
        return false;
      }
    }
    scratch = std::move(view.scratch);
    payload = view.payload.data() == scratch.data() ? std::span<const uint8_t>{ scratch } : view.payload;
    return true;
  }

  void peer_mesh::deliver_or_forward(natural_t link_id, node_id src, node_id dst, uint8_t ttl, bool routed,
                                     uint16_t net_id, std::span<const uint8_t> payload) {
    link_record* record = links.mutable_link(link_id);
    if (record == nullptr) {
      return;
    }

    ostd::vector<uint8_t> scratch;
    if (!run_filters(rx_filters, *record, src, dst, net_id, payload, scratch)) {
      return;
    }

    /// a frame on link L is at L.local's seat: deliver only when this seat IS the destination —
    ///  a resident dst elsewhere in the mesh must still be reached over links (hops stay real)
    if (dst == record->local) {
      peer_mesh_actor* destination = actor(dst);
      if (destination == nullptr) {
        stats.no_actor_drops++;
        return;
      }
      const link_record via = *record;  // stable copy: the actor may mutate topology
      destination->on_frame(via, src, net_id, payload);
      return;
    }

    if (!routed) {
      stats.no_actor_drops++;
      return;
    }
    forward(link_id, route_header{ .src = src, .dst = dst, .ttl = ttl }, net_id, payload);
  }

  void peer_mesh::forward(natural_t in_link_id, const route_header& route, uint16_t net_id, std::span<const uint8_t> payload) {
    link_record* in_link = links.mutable_link(in_link_id);
    if (in_link == nullptr) {
      return;
    }
    if (route.ttl == 0) {
      in_link->stats.ttl_drops++;
      return;
    }

    const node_id relay = in_link->local;
    const opt<natural_t> hop = active_router->next_hop(relay, route.dst);
    if (!hop.has_value()) {
      in_link->stats.drops++;
      stats.unroutable_drops++;
      return;
    }

    const link_record* out_link = links.link(*hop);
    if (out_link == nullptr || out_link->state != link_state::UP) {
      in_link->stats.drops++;
      stats.unroutable_drops++;
      return;
    }

    const route_header next{ .src = route.src, .dst = route.dst, .ttl = static_cast<uint8_t>(route.ttl - 1) };
    transmit(*hop, net_id, payload, &next);
  }

  /// ---------------------------------------------------------------- control

  void peer_mesh::handle_control(natural_t link_id, uint16_t net_id, std::span<const uint8_t> payload) {
    link_record* record = links.mutable_link(link_id);
    if (record == nullptr) {
      return;
    }

    try {
      switch (static_cast<mesh_message>(net_id)) {
        case mesh_message::LINK_HELLO:
          handle_hello(link_id, payload);
          return;

        case mesh_message::LINK_PING: {
          auto [ping, consumed] = deserialize_direct<mesh_link_ping>(payload);
          const mesh_link_pong pong{ .t_echo_us = ping.t_send_us };
          send_control(link_id, mesh_message::LINK_PONG, serialize_direct(pong));
          return;
        }

        case mesh_message::LINK_PONG: {
          auto [pong, consumed] = deserialize_direct<mesh_link_pong>(payload);
          network::link_runtime* rt = links.runtime(link_id);
          if (rt == nullptr || !rt->ping_outstanding || pong.t_echo_us != rt->ping_token) {
            return;
          }
          rt->ping_outstanding = false;
          const microseconds sample = current_now - rt->ping_sent_at;
          record->rtt = record->rtt.count() == 0 ? sample : (record->rtt * 7 + sample) / 8;
          if (peer_record* node = nodes.record(record->remote); node != nullptr) {
            node->rtt_estimate = record->rtt;
          }
          return;
        }

        case mesh_message::LINK_BYE: {
          auto [bye, consumed] = deserialize_direct<mesh_link_bye>(payload);
          CORE_LOG_TRACE("[MESH {}] link {} remote bye (reason {})", debug_name, link_id, bye.reason);
          teardown(link_id, link_close_reason::REMOTE_BYE, false);
          return;
        }

        case mesh_message::LINK_AUTH: {
          if (link_sec == nullptr || record->state != link_state::AUTHENTICATING) {
            protocol_error(link_id, "unexpected LINK_AUTH");
            return;
          }
          apply_auth_result(link_id, link_sec->on_auth_frame(*this, *record, payload));
          return;
        }
      }
      protocol_error(link_id, "unknown mesh control id");
    } catch (const std::exception& error) {
      CORE_LOG_WARN("[MESH {}] link {} control parse failure: {}", debug_name, link_id, error.what());
      protocol_error(link_id, "control payload parse failure");
    }
  }

  void peer_mesh::handle_hello(natural_t link_id, std::span<const uint8_t> payload) {
    link_record* record = links.mutable_link(link_id);
    network::link_runtime* rt = links.runtime(link_id);
    if (record == nullptr || rt == nullptr) {
      return;
    }
    if (record->state != link_state::HANDSHAKING || rt->remote_hello_valid) {
      protocol_error(link_id, "unexpected LINK_HELLO");
      return;
    }

    auto [hello, consumed] = deserialize_direct<mesh_link_hello>(payload);
    if (hello.magic != kMeshMagic || hello.protocol != kMeshProtocolVersion) {
      protocol_error(link_id, "hello magic/protocol mismatch");
      return;
    }
    if (hello.app_hash != cfg.app_hash) {
      protocol_error(link_id, "hello app mismatch");
      return;
    }
    if (hello.node == 0) {
      protocol_error(link_id, "hello carries invalid node id");
      return;
    }

    /// D13 hardening: a transport that authenticates its remote (steam) pins the
    ///  hello — a mismatched claim is an identity failure, not a protocol slip
    if (const node_id attested = transports[rt->transport_index].transport->attested_remote(record->connection_id); attested != 0) {
      if (hello.node != attested) {
        stats.security_failures++;
        CORE_LOG_WARN("[MESH {}] link {} hello claims node {:#x} but transport attests {:#x}", debug_name, link_id, hello.node, attested);
        teardown(link_id, link_close_reason::SECURITY_ERROR, true);
        return;
      }
      record->attested = true;
    }

    record->remote = hello.node;
    record->caps.reliable = record->caps.reliable && hello.reliable != 0;
    record->caps.ordered = record->caps.ordered && hello.ordered != 0;
    if (hello.max_frame_size != 0) {
      record->caps.max_frame_size = record->caps.max_frame_size == 0
        ? hello.max_frame_size
        : std::min(record->caps.max_frame_size, hello.max_frame_size);
    }
    rt->remote_hello_valid = true;

    try_advance_past_handshake(link_id);
  }

  void peer_mesh::try_advance_past_handshake(natural_t link_id) {
    link_record* record = links.mutable_link(link_id);
    network::link_runtime* rt = links.runtime(link_id);
    if (record == nullptr || rt == nullptr || !rt->hello_sent || !rt->remote_hello_valid) {
      return;
    }

    if (link_sec == nullptr) {
      make_link_up(link_id);
      return;
    }

    record->state = link_state::AUTHENTICATING;
    apply_auth_result(link_id, link_sec->begin_auth(*this, *record));
  }

  void peer_mesh::apply_auth_result(natural_t link_id, link_security::auth_result result) {
    switch (result) {
      case link_security::auth_result::PENDING:
        return;
      case link_security::auth_result::ESTABLISHED:
        make_link_up(link_id);
        return;
      case link_security::auth_result::FAILED:
        stats.security_failures++;
        teardown(link_id, link_close_reason::SECURITY_ERROR, true);
        return;
    }
  }

  void peer_mesh::make_link_up(natural_t link_id) {
    link_record* record = links.mutable_link(link_id);
    if (record == nullptr) {
      return;
    }
    record->state = link_state::UP;

    peer_record& remote = nodes.ensure_node(record->remote);
    remote.last_seen_tick = tick_counter;
    nodes.add_edge(record->local, record->remote);

    active_router->on_link_up(*record);
    CORE_LOG_TRACE("[MESH {}] link {} up ({} <-> {})", debug_name, link_id, record->local, record->remote);

    const link_record snapshot = *record;
    if (peer_mesh_actor* local = actor(snapshot.local); local != nullptr) {
      local->on_link_up(snapshot);
    }
  }

  /// ---------------------------------------------------------------- tx

  void peer_mesh::send_hello(natural_t link_id) {
    link_record* record = links.mutable_link(link_id);
    network::link_runtime* rt = links.runtime(link_id);
    if (record == nullptr || rt == nullptr || rt->hello_sent) {
      return;
    }

    const mesh_link_hello hello{
      .node = record->local,
      .app_hash = cfg.app_hash,
      .reliable = static_cast<uint8_t>(record->caps.reliable ? 1 : 0),
      .ordered = static_cast<uint8_t>(record->caps.ordered ? 1 : 0),
      .max_frame_size = cfg.max_frame_size,
    };
    rt->hello_sent = true;
    send_control(link_id, mesh_message::LINK_HELLO, serialize_direct(hello));
    try_advance_past_handshake(link_id);
  }

  void peer_mesh::send_control(natural_t link_id, mesh_message id, std::span<const uint8_t> payload) {
    link_record* record = links.mutable_link(link_id);
    network::link_runtime* rt = links.runtime(link_id);
    if (record == nullptr || rt == nullptr) {
      return;
    }

    /// control frames skip filters and encryption by design
    const ostd::vector<uint8_t> frame = write_frame(control_id(id), payload);
    transports[rt->transport_index].transport->tx(record->connection_id, frame);
    rt->last_tx = current_now;
    record->latest_tx_tick = tick_counter;
    record->stats.frames_tx++;
    record->stats.bytes_tx += frame.size();
  }

  void peer_mesh::send_link_auth(link_record& link, std::span<const uint8_t> blob) {
    send_control(link.link_id, mesh_message::LINK_AUTH, blob);
  }

  bool peer_mesh::transmit(natural_t link_id, uint16_t net_id, std::span<const uint8_t> payload, const route_header* route) {
    PROFILE_SECTION("peer_mesh::transmit");
    link_record* record = links.mutable_link(link_id);
    network::link_runtime* rt = links.runtime(link_id);
    if (record == nullptr || rt == nullptr || record->state != link_state::UP) {
      stats.refused_sends++;
      return false;
    }

    const node_id src = route != nullptr ? route->src : record->local;
    const node_id dst = route != nullptr ? route->dst : record->remote;

    ostd::vector<uint8_t> scratch;
    std::span<const uint8_t> filtered = payload;
    if (!run_filters(tx_filters, *record, src, dst, net_id, filtered, scratch)) {
      return true;  // a filter consumed the frame; that is its prerogative
    }

    /// compose [route header?][payload], then encrypt the whole thing — per-link
    ///  security covers the envelope, relays re-protect per hop
    ostd::vector<uint8_t> composed;
    uint16_t flags = static_cast<uint16_t>(frame_flags::NONE);
    if (route != nullptr) {
      composed.resize(kRouteHeaderSize + filtered.size());
      write_route_header(*route, composed.data());
      std::ranges::copy(filtered, composed.data() + kRouteHeaderSize);
      flags = static_cast<uint16_t>(frame_flags::ROUTED);
    } else {
      composed.assign(filtered.begin(), filtered.end());
    }

    if (link_sec != nullptr && !link_sec->encrypt(*record, composed)) {
      stats.security_failures++;
      teardown(link_id, link_close_reason::SECURITY_ERROR, true);
      return false;
    }

    const uint32_t frame_length = static_cast<uint32_t>(kFrameLengthFloor + composed.size());
    const uint32_t limit = record->caps.max_frame_size != 0 ? std::min(record->caps.max_frame_size, cfg.max_frame_size) : cfg.max_frame_size;
    if (frame_length > limit) {
      CORE_LOG_WARN("[MESH {}] link {}: refusing oversize send ({} > {})", debug_name, link_id, frame_length, limit);
      stats.refused_sends++;
      return false;
    }

    const ostd::vector<uint8_t> frame = write_flagged_frame(net_id, flags, composed);
    transports[rt->transport_index].transport->tx(record->connection_id, frame);
    rt->last_tx = current_now;
    record->latest_tx_tick = tick_counter;
    record->stats.frames_tx++;
    record->stats.bytes_tx += frame.size();
    return true;
  }

  bool peer_mesh::send_from(peer_mesh_actor& from, node_id dst, uint16_t net_id, std::span<const uint8_t> payload) {
    if (is_mesh_control(net_id)) {
      stats.refused_sends++;
      return false;
    }

    if (const link_record* direct = links.link_between(from.node, dst); direct != nullptr) {
      return transmit(direct->link_id, net_id, payload, nullptr);
    }

    const opt<natural_t> hop = active_router->next_hop(from.node, dst);
    if (!hop.has_value()) {
      stats.unroutable_drops++;
      return false;
    }
    const link_record* out = links.link(*hop);
    if (out == nullptr || out->state != link_state::UP || out->local != from.node) {
      stats.unroutable_drops++;
      return false;
    }

    const route_header route{ .src = from.node, .dst = dst, .ttl = cfg.default_ttl };
    return transmit(*hop, net_id, payload, &route);
  }

  bool peer_mesh::send_on_link_from(peer_mesh_actor& from, natural_t link_id, uint16_t net_id, std::span<const uint8_t> payload) {
    if (is_mesh_control(net_id)) {
      stats.refused_sends++;
      return false;
    }
    const link_record* record = links.link(link_id);
    if (record == nullptr || record->local != from.node) {
      stats.refused_sends++;
      return false;
    }
    return transmit(link_id, net_id, payload, nullptr);
  }

  /// ---------------------------------------------------------------- lifecycle

  void peer_mesh::protocol_error(natural_t link_id, std::string_view what) {
    stats.protocol_errors++;
    CORE_LOG_WARN("[MESH {}] link {} protocol error: {}", debug_name, link_id, what);
    teardown(link_id, link_close_reason::PROTOCOL_ERROR, true);
  }

  link_transport* peer_mesh::transport_of(natural_t link_id) {
    network::link_runtime* rt = links.runtime(link_id);
    return rt != nullptr ? transports[rt->transport_index].transport : nullptr;
  }

  void peer_mesh::teardown(natural_t link_id, link_close_reason reason, bool send_bye) {
    link_record* record = links.mutable_link(link_id);
    if (record == nullptr) {
      return;
    }
    if (record->state == link_state::DOWN || record->state == link_state::DISCONNECTING) {
      return;
    }

    record->state = link_state::DISCONNECTING;
    if (send_bye) {
      /// best effort: bypasses transmit's UP gate by construction (control path)
      const mesh_link_bye bye{ .reason = static_cast<uint16_t>(reason) };
      send_control(link_id, mesh_message::LINK_BYE, serialize_direct(bye));
    }

    const link_record snapshot = *record;
    if (link_transport* t = transport_of(link_id); t != nullptr) {
      t->close(snapshot.connection_id);
    }

    active_router->on_link_down(snapshot);
    nodes.remove_edge(snapshot.local, snapshot.remote);
    links.drop(link_id);

    CORE_LOG_TRACE("[MESH {}] link {} down (reason {})", debug_name, link_id, static_cast<uint16_t>(reason));
    if (peer_mesh_actor* local = actor(snapshot.local); local != nullptr) {
      local->on_link_down(snapshot, reason);
    }
  }

  void peer_mesh::tick(microseconds now) {
    PROFILE_SECTION("peer_mesh::tick");
    const double dt = ticked_once ? duration_cast<fseconds>(now - current_now).count() : 0.0;
    current_now = now;
    ticked_once = true;
    tick_counter++;

    /// timeouts over a stable id snapshot — teardown mutates the table
    ostd::vector<natural_t> ids;
    for (const link_record& record : links.links()) {
      ids.push_back(record.link_id);
    }
    for (const natural_t link_id : ids) {
      link_record* record = links.mutable_link(link_id);
      network::link_runtime* rt = links.runtime(link_id);
      if (record == nullptr || rt == nullptr) {
        continue;
      }

      switch (record->state) {
        case link_state::CONNECTING:
        case link_state::HANDSHAKING:
        case link_state::AUTHENTICATING:
          if (now - rt->opened_at > cfg.handshake_timeout) {
            teardown(link_id, link_close_reason::HANDSHAKE_TIMEOUT, true);
          }
          break;

        case link_state::UP:
          if (now - rt->last_rx > cfg.link_timeout) {
            teardown(link_id, link_close_reason::KEEPALIVE_TIMEOUT, true);
            break;
          }
          if (!rt->ping_outstanding && now - rt->last_tx > cfg.keepalive_idle) {
            rt->ping_outstanding = true;
            rt->ping_sent_at = now;
            rt->ping_token = static_cast<uint64_t>(now.count());
            const mesh_link_ping ping{ .t_send_us = rt->ping_token };
            send_control(link_id, mesh_message::LINK_PING, serialize_direct(ping));
          }
          break;

        case link_state::DISCONNECTING:
        case link_state::DOWN:
          break;
      }
    }

    /// actor ticks over a stable snapshot — actors may spawn/destroy/link
    ostd::vector<node_id> actor_ids;
    for (const scope<peer_mesh_actor>& a : resident_actors) {
      actor_ids.push_back(a->node);
    }
    for (const node_id id : actor_ids) {
      if (peer_mesh_actor* a = actor(id); a != nullptr) {
        a->tick(now, dt);
      }
    }
  }

}  // namespace other

/**
 * \file peer_mesh/peer_mesh.cpp
 **/
#include "peer_mesh/peer_mesh.hpp"

#include <algorithm>

#include "core/logger.hpp"
#include "core/profiler.hpp"

#include "network/link_sink.hpp"
#include "network/mesh_messages.hpp"
#include "network/transport_provider.hpp"

namespace other {

  peer_mesh::peer_mesh(std::string_view debug_name, const peer_mesh_config& config)
      : debug_name(debug_name), cfg(config) {}

  peer_mesh::~peer_mesh() {
    /// close every live link; providers outlive the mesh (not owned)
    ostd::vector<natural_t> live;
    for (const auto& [link_id, s] : link_sinks) {
      live.push_back(link_id);
    }
    for (const natural_t id : live) {
      close_link(id, link_close_reason::SHUTDOWN);
    }

    /// accept delegates capture this mesh — they must not outlive it
    for (const auto& [key, owner] : listener_owners) {
      transports[key.first].provider->release_listener(key.second);
    }
  }

  void peer_mesh::attach_provider(transport_provider& provider) {
    std::string lowered;
    for (const char c : provider.name()) {
      lowered.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    }
    for (const transport_entry& entry : transports) {
      OTHER_ASSERT(entry.provider != &provider, "Provider '{}' is already attached to mesh '{}'.", lowered, debug_name);
      OTHER_ASSERT(entry.lowered_name != lowered, "A provider named '{}' is already attached to mesh '{}'.", lowered, debug_name);
    }
    transports.push_back({ &provider, std::move(lowered) });
  }

  std::string_view peer_mesh::transport_name(natural_t transport_hash) const {
    for (const transport_entry& entry : transports) {
      if (entry.provider->hash() == transport_hash) {
        return entry.lowered_name;
      }
    }
    return "";
  }

  /// ---------------------------------------------------------------- actors

  peer_actor& peer_mesh::spawn_common(scope<peer_actor> actor, node_id id, bool primary, bool auto_spawned) {
    OTHER_ASSERT(actor != nullptr, "Cannot spawn a null actor.");
    OTHER_ASSERT(id != 0, "Actor node id 0 is reserved for invalid/unknown.");
    OTHER_ASSERT(this->actor(id) == nullptr, "An actor with node id {} is already resident in mesh '{}'.", id, debug_name);

    actor->owner = this;
    actor->node = id;
    actor->primary = primary;
    actor->auto_spawned = auto_spawned;

    resident_actors.push_back(std::move(actor));
    return *resident_actors.back();
  }

  peer_actor& peer_mesh::set_primary(scope<peer_actor> actor, node_id id) {
    OTHER_ASSERT(primary_id == 0, "Mesh '{}' already has a primary actor (node {}).", debug_name, primary_id);
    peer_actor& spawned = spawn_common(std::move(actor), id, true, false);
    primary_id = id;
    return spawned;
  }

  peer_actor& peer_mesh::add_secondary(scope<peer_actor> actor, node_id id) {
    OTHER_ASSERT(primary_id != 0, "Mesh '{}' needs a primary before secondaries.", debug_name);
    return spawn_common(std::move(actor), id, false, false);
  }

  void peer_mesh::set_secondary_factory(std::function<scope<peer_actor>(const link_record&)> fn) {
    secondary_factory = std::move(fn);
  }

  void peer_mesh::remove_actor(node_id id) {
    auto itr = std::ranges::find_if(resident_actors, [id](const scope<peer_actor>& a) { return a->node == id; });
    if (itr == resident_actors.end()) {
      CORE_LOG_WARN("[MESH {}] remove_actor: no resident actor with node id {}", debug_name, id);
      return;
    }

    ostd::vector<natural_t> owned;
    for (const auto& [link_id, s] : link_sinks) {
      if (s->link().local == id) {
        owned.push_back(link_id);
      }
    }
    for (const natural_t link_id : owned) {
      close_link(link_id, link_close_reason::ACTOR_DESTROYED);
    }

    /// listeners owned by this actor stop accepting into the mesh
    for (auto entry = listener_owners.begin(); entry != listener_owners.end();) {
      if (entry->second == id) {
        transports[entry->first.first].provider->release_listener(entry->first.second);
        entry = listener_owners.erase(entry);
      } else {
        ++entry;
      }
    }

    itr = std::ranges::find_if(resident_actors, [id](const scope<peer_actor>& a) { return a->node == id; });
    if (itr != resident_actors.end()) {
      (*itr)->owner = nullptr;
      resident_actors.erase(itr);
    }
    if (primary_id == id) {
      primary_id = 0;
    }
  }

  peer_actor* peer_mesh::actor(node_id id) {
    auto itr = std::ranges::find_if(resident_actors, [id](const scope<peer_actor>& a) { return a->node == id; });
    return itr != resident_actors.end() ? itr->get() : nullptr;
  }

  void peer_mesh::reap_auto_secondary(node_id id) {
    peer_actor* candidate = actor(id);
    if (candidate == nullptr || !candidate->auto_spawned) {
      return;
    }
    for (const auto& [link_id, s] : link_sinks) {
      const link_record& record = s->link();
      if (record.local == id || record.remote == id) {
        return;
      }
    }
    remove_actor(id);
  }

  void peer_mesh::set_security(scope<link_security> new_security) {
    OTHER_ASSERT(link_count() == 0, "Security layer must be installed before any link exists.");
    link_sec = std::move(new_security);
  }

  /// ---------------------------------------------------------------- link table

  link_sink* peer_mesh::sink(natural_t link_id) {
    auto itr = link_sinks.find(link_id);
    return itr != link_sinks.end() ? itr->second : nullptr;
  }

  const link_sink* peer_mesh::sink(natural_t link_id) const {
    auto itr = link_sinks.find(link_id);
    return itr != link_sinks.end() ? itr->second : nullptr;
  }

  ostd::vector<link_record> peer_mesh::links() const {
    ostd::vector<link_record> out;
    out.reserve(link_sinks.size());
    for (const auto& [link_id, s] : link_sinks) {
      out.push_back(s->link());
    }
    return out;
  }

  const link_record* peer_mesh::link(natural_t link_id) const {
    const link_sink* s = sink(link_id);
    return s != nullptr ? &s->link() : nullptr;
  }

  const link_record* peer_mesh::link_between(node_id local, node_id remote) const {
    for (const auto& [link_id, s] : link_sinks) {
      const link_record& record = s->link();
      if (record.local == local && record.remote == remote && record.state == link_state::UP) {
        return &record;
      }
    }
    return nullptr;
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
      if (transports[i].lowered_name == wanted) {
        out_index = i;
        return 1;
      }
    }
    CORE_LOG_WARN("[MESH {}] no transport '{}' attached", debug_name, wanted);
    return 0;
  }

  natural_t peer_mesh::open_link_from(peer_actor& from, const net_address& remote, std::string_view transport_name) {
    PROFILE_SECTION("peer_mesh::open_link");
    if (link_count() >= cfg.max_links) {
      CORE_LOG_WARN("[MESH {}] open_link refused: at max links ({})", debug_name, cfg.max_links);
      stats.refused_sends++;
      return 0;
    }

    size_t transport_index = 0;
    if (resolve_transport(remote, transport_name, transport_index) == 0) {
      return 0;
    }

    transport_provider& provider = *transports[transport_index].provider;
    const natural_t conn_id = provider.dial(remote);
    if (conn_id == 0) {
      return 0;
    }
    link_sink* dialed = provider.sink_of(conn_id);
    if (dialed == nullptr) {
      CORE_LOG_WARN("[MESH {}] dial produced no link sink on '{}'", debug_name, transports[transport_index].lowered_name);
      return 0;
    }

    const natural_t link_id = adopt_sink(*dialed, transport_index, from.node, link_state::CONNECTING);
    CORE_LOG_TRACE("[MESH {}] link {} dialing via '{}' (local node {})", debug_name, link_id, transports[transport_index].lowered_name, from.node);
    return link_id;
  }

  natural_t peer_mesh::open_listener_from(peer_actor& from, const net_address& bind, std::string_view transport_name) {
    size_t transport_index = 0;
    if (resolve_transport(bind, transport_name, transport_index) == 0) {
      return 0;
    }

    transport_provider& provider = *transports[transport_index].provider;
    const natural_t listener_id = provider.listen(bind, [this, transport_index](link_sink& accepted, natural_t listener) {
      handle_accept(transport_index, accepted, listener);
    });
    if (listener_id == 0) {
      return 0;
    }

    listener_owners[{ transport_index, listener_id }] = from.node;
    return listener_id;
  }

  natural_t peer_mesh::adopt_sink(link_sink& adopted, size_t transport_index, node_id local_seat, link_state initial) {
    const natural_t link_id = next_link_id++;
    link_sinks[link_id] = &adopted;

    link_sink::settings settings{
      .app_hash = cfg.app_hash,
      .max_frame_size = cfg.max_frame_size,
      .handshake_timeout = cfg.handshake_timeout,
      .keepalive_idle = cfg.keepalive_idle,
      .link_timeout = cfg.link_timeout,
    };
    link_sink* sink_ptr = &adopted;
    adopted.adopt(link_id, local_seat, initial, settings, link_sec.get(), &stats,
                  link_sink::events{
                    .link_up = [this](link_record& record) { on_sink_up(record); },
                    .link_down = [this, sink_ptr](const link_record& snapshot, link_close_reason reason) {
                      on_sink_down(*sink_ptr, snapshot, reason);
                    },
                    .frame = [this](const link_record& record, uint16_t net_id, std::span<const uint8_t> payload) {
                      on_sink_frame(record, net_id, payload);
                    },
                  },
                  current_now);
    return link_id;
  }

  void peer_mesh::handle_accept(size_t transport_index, link_sink& accepted, natural_t listener_id) {
    auto owner = listener_owners.find({ transport_index, listener_id });
    transport_provider& provider = *transports[transport_index].provider;
    if (owner == listener_owners.end() || link_count() >= cfg.max_links) {
      provider.close(accepted.conn_id());
      provider.release_sink(accepted.conn_id());
      return;
    }
    adopt_sink(accepted, transport_index, owner->second, link_state::HANDSHAKING);
  }

  ostd::vector<link_record> peer_mesh::links_of(node_id local) const {
    ostd::vector<link_record> out;
    for (const auto& [link_id, s] : link_sinks) {
      if (s->link().local == local) {
        out.push_back(s->link());
      }
    }
    return out;
  }

  void peer_mesh::close_link(natural_t link_id, link_close_reason reason) {
    if (link_sink* s = sink(link_id); s != nullptr) {
      s->begin_close(reason, true);
    }
  }

  /// ---------------------------------------------------------------- sink events

  void peer_mesh::on_sink_up(link_record& record) {
    /// an unknown remote materializes as a secondary — the mesh's record of the far
    ///  endpoint. simulated remotes were added explicitly and are found instead
    if (actor(record.remote) == nullptr) {
      scope<peer_actor> minted = secondary_factory ? secondary_factory(record) : make_scope<peer_actor>();
      if (minted != nullptr) {
        spawn_common(std::move(minted), record.remote, false, true);
      }
    }

    CORE_LOG_TRACE("[MESH {}] link {} up ({} <-> {})", debug_name, record.link_id, record.local, record.remote);

    const link_record snapshot = record;
    if (peer_actor* local = actor(snapshot.local); local != nullptr) {
      local->on_link_up(snapshot);
    }
  }

  void peer_mesh::on_sink_down(link_sink& closed, const link_record& snapshot, link_close_reason reason) {
    link_sinks.erase(snapshot.link_id);
    closed.provider().release_sink(closed.conn_id());

    CORE_LOG_TRACE("[MESH {}] link {} down (reason {})", debug_name, snapshot.link_id, static_cast<uint16_t>(reason));
    if (peer_actor* local = actor(snapshot.local); local != nullptr) {
      local->on_link_down(snapshot, reason);
    }
    if (snapshot.remote != 0) {
      reap_auto_secondary(snapshot.remote);
    }
  }

  void peer_mesh::on_sink_frame(const link_record& record, uint16_t net_id, std::span<const uint8_t> payload) {
    /// the one delivery rule: a frame on link L belongs to the actor at L's seat
    peer_actor* destination = actor(record.local);
    if (destination == nullptr) {
      stats.no_actor_drops++;
      return;
    }
    const link_record snapshot = record;  // stable copy: the actor may mutate topology
    destination->on_frame(snapshot, snapshot.remote, net_id, payload);
  }

  /// ---------------------------------------------------------------- tx

  bool peer_mesh::transmit(natural_t link_id, uint16_t net_id, std::span<const uint8_t> payload) {
    PROFILE_SECTION("peer_mesh::transmit");
    link_sink* out = sink(link_id);
    if (out == nullptr || out->link().state != link_state::UP) {
      stats.refused_sends++;
      return false;
    }
    return out->tx(net_id, payload);
  }

  bool peer_mesh::send_from(peer_actor& from, node_id dst, uint16_t net_id, std::span<const uint8_t> payload) {
    if (is_mesh_control(net_id)) {
      stats.refused_sends++;
      return false;
    }
    const link_record* direct = link_between(from.node, dst);
    if (direct == nullptr) {
      stats.refused_sends++;
      return false;
    }
    return transmit(direct->link_id, net_id, payload);
  }

  bool peer_mesh::send_on_link_from(peer_actor& from, natural_t link_id, uint16_t net_id, std::span<const uint8_t> payload) {
    if (is_mesh_control(net_id)) {
      stats.refused_sends++;
      return false;
    }
    const link_record* record = link(link_id);
    if (record == nullptr || record->local != from.node) {
      stats.refused_sends++;
      return false;
    }
    return transmit(link_id, net_id, payload);
  }

  /// ---------------------------------------------------------------- tick

  void peer_mesh::tick(microseconds now) {
    PROFILE_SECTION("peer_mesh::tick");
    const double dt = ticked_once ? duration_cast<fseconds>(now - current_now).count() : 0.0;
    current_now = now;
    ticked_once = true;
    tick_counter++;

    /// provider maintenance first: queued accepts adopt, released sinks reclaim
    for (const transport_entry& entry : transports) {
      entry.provider->maintain();
    }

    /// pump over a stable id snapshot — a sink may tear its link down mid-pump
    ostd::vector<natural_t> ids;
    for (const auto& [link_id, s] : link_sinks) {
      ids.push_back(link_id);
    }
    for (const natural_t link_id : ids) {
      if (link_sink* s = sink(link_id); s != nullptr) {
        s->pump(now);
      }
    }

    /// actor ticks over a stable snapshot — actors may spawn/destroy/link
    ostd::vector<node_id> actor_ids;
    for (const scope<peer_actor>& a : resident_actors) {
      actor_ids.push_back(a->node);
    }
    for (const node_id id : actor_ids) {
      if (peer_actor* a = actor(id); a != nullptr) {
        a->tick(now, dt);
      }
    }
  }

}  // namespace other

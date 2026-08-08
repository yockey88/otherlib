/**
 * \file network/session/replication.cpp
 **/
#include "network/session/replication.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/norm.hpp>

#include "core/logger.hpp"
#include "core/profiler.hpp"

#include "object/network_component.hpp"
#include "object/transform.hpp"
#include "serialization/component_codec.hpp"
#include "serialization/scene_serializer.hpp"

namespace other {

  namespace {

    bool transform_dirty(const transform& current, const transform& last_sent, float epsilon) {
      return glm::length2(current.local_position - last_sent.local_position) > epsilon ||
             glm::length2(current.local_scale - last_sent.local_scale) > epsilon ||
             glm::abs(1.f - glm::abs(glm::dot(current.local_rotation_quat, last_sent.local_rotation_quat))) > epsilon;
    }

  }  // namespace

  replication::replication(network_session& session, std::function<scene*()> scene_source, const replication_config& cfg)
      : session(session), scene_source(std::move(scene_source)), cfg(cfg) {
    const auto with_scene = [this](void (replication::*handler)(scene&, std::span<const uint8_t>)) {
      return [this, handler](node_id, std::span<const uint8_t> payload) {
        if (scene* s = live_scene(); s != nullptr) {
          (this->*handler)(*s, payload);
        }
      };
    };
    session.register_handler(net_message::JOIN_SNAPSHOT, with_scene(&replication::handle_join_snapshot));
    session.register_handler(net_message::SPAWN, with_scene(&replication::handle_spawn));
    session.register_handler(net_message::DESPAWN, with_scene(&replication::handle_despawn));
    session.register_handler(net_message::TRANSFORM_BATCH, with_scene(&replication::handle_transform_batch));
    session.register_handler(net_message::COMPONENT_STATE, with_scene(&replication::handle_component_state));
  }

  void replication::on_session_event(session_event ev, uint16_t arg) {
    scene* s = live_scene();
    switch (ev) {
      case session_event::STARTED:
        if (s != nullptr) {
          s->network().role = session.is_host() ? replication_role::AUTHORITY : replication_role::REPLICA;
          if (session.is_host()) {
            sweep_authored(*s);
          }
        }
        return;

      case session_event::ENDED:
        end_session_locally();
        return;

      case session_event::PEER_JOINED:
        if (s != nullptr && session.is_host()) {
          sweep_authored(*s);
          send_join_snapshot(*s, arg);
        }
        return;

      case session_event::PEER_LEFT:
        if (s != nullptr && session.is_host()) {
          despawn_owned_by(*s, arg);
        }
        return;
    }
  }

  void replication::tick(microseconds now) {
    session_now = now;
    scene* s = live_scene();
    if (s == nullptr || !session.in_session()) {
      return;
    }

    if (s->network().role == replication_role::AUTHORITY) {
      const microseconds interval{ static_cast<int64_t>(1'000'000.0 / cfg.snapshot_hz) };
      if (now - last_snapshot >= interval) {
        last_snapshot = now;
        snapshot_pass(*s);
      }
      return;
    }
    if (s->network().role == replication_role::REPLICA) {
      apply_interpolation(*s);
    }
  }

  bool replication::spawn_object(natural_t object_id, uint16_t owner_peer) {
    scene* s = live_scene();
    if (s == nullptr || s->network().role != replication_role::AUTHORITY) {
      CORE_LOG_WARN("[REPL] spawn refused: not the session authority");
      return false;
    }
    if (s->find_object(object_id) == nullptr) {
      CORE_LOG_WARN("[REPL] spawn refused: no object {}", object_id);
      return false;
    }
    s->network().register_object(object_id, owner_peer);
    return true;
  }

  bool replication::sync_component(natural_t object_id, natural_t key_hash) {
    scene* s = live_scene();
    if (s == nullptr || s->network().role != replication_role::AUTHORITY) {
      CORE_LOG_WARN("[REPL] sync refused: not the session authority");
      return false;
    }
    const opt<natural_t> net_id = s->network().net_of(object_id);
    if (!net_id.has_value()) {
      CORE_LOG_WARN("[REPL] sync refused: object {} is not replicated", object_id);
      return false;
    }
    pending_syncs.emplace_back(*net_id, key_hash);
    return true;
  }

  bool replication::is_mine(natural_t object_id) const {
    scene* s = live_scene();
    if (s == nullptr || s->network().role == replication_role::NONE) {
      return false;
    }
    for (const net_object_entry& entry : s->network().entries()) {
      if (entry.object_id == object_id) {
        return entry.owner_peer == session.local_peer_id();
      }
    }
    return false;
  }

  /// ---------------------------------------------------------------- host side

  void replication::sweep_authored(scene& s) {
    PROFILE_SECTION("replication::sweep_authored");
    /// authored network_components self-register on the authority — this also
    ///  rebuilds the registry after a play/stop restore reminted every runtime id
    for (const natural_t object_id : s.get_all_object_ids()) {
      if (const network_component* net = s.get_component<network_component>(object_id); net != nullptr) {
        if (!s.network().net_of(object_id).has_value()) {
          s.network().register_object(object_id, static_cast<uint16_t>(net->owner_peer));
        }
      }
    }
  }

  void replication::send_join_snapshot(scene& s, uint16_t peer_id) {
    PROFILE_SECTION("replication::send_join_snapshot");
    net_join_snapshot msg;
    msg.host_tick = snapshot_tick;
    msg.scene_bytes = serialization::write_scene_binary(serialization::capture_scene(s, serialization::default_codec_services()));
    for (const net_object_entry& entry : s.network().entries()) {
      /// live capture: object_record.file_id == the runtime id
      msg.table.push_back({ .net_id = entry.net_id, .file_id = entry.object_id, .owner_peer = entry.owner_peer });
    }
    session.send(peer_id, net_message::JOIN_SNAPSHOT, serialize_direct(msg));
  }

  void replication::snapshot_pass(scene& s) {
    PROFILE_SECTION("replication::snapshot_pass");
    snapshot_tick++;
    sweep_authored(s);

    /// broadcast order: spawns -> component states -> despawns -> transform batch
    net_transform_batch batch;
    batch.host_tick = snapshot_tick;
    ostd::vector<natural_t> dead;

    for (net_object_entry& entry : s.network().entries()) {
      scene_object* object = s.find_object(entry.object_id);
      if (object == nullptr) {
        dead.push_back(entry.net_id);
        continue;
      }

      if (!entry.spawn_sent) {
        net_spawn msg{ .net_id = entry.net_id, .owner_peer = entry.owner_peer };
        msg.name = ostd::vector<uint8_t>(object->name.begin(), object->name.end());
        if (const scene_object* parent = s.get_parent(entry.object_id); parent != nullptr) {
          msg.parent_net_id = s.network().net_of(parent->id).value_or(0);
        }
        for (const serialization::component_codec& codec : serialization::component_codecs()) {
          if (!codec.has(s, object)) {
            continue;
          }
          net_component_blob blob{ .key_hash = codec.key_hash };
          blob.payload = codec.capture(s, object, serialization::default_codec_services());
          if (codec.implicit && blob.payload.empty()) {
            continue;
          }
          msg.components.append_range(serialize_direct(blob));
          msg.component_count++;
        }
        session.broadcast(net_message::SPAWN, serialize_direct(msg));
        entry.spawn_sent = true;
      }

      if (entry.replicate_transform) {
        if (const transform* trs = s.get_component<transform>(object); trs != nullptr &&
            transform_dirty(*trs, entry.last_sent, cfg.transform_epsilon)) {
          entry.last_sent = *trs;
          batch.entries.push_back({ .net_id = entry.net_id,
                                    .position = trs->local_position,
                                    .rotation = trs->local_rotation_quat,
                                    .scale = trs->local_scale });
        }
      }
    }

    for (const auto& [net_id, key_hash] : pending_syncs) {
      const opt<natural_t> object_id = s.network().object_of(net_id);
      const serialization::component_codec* codec = serialization::find_component_codec(key_hash);
      scene_object* object = object_id.has_value() ? s.find_object(*object_id) : nullptr;
      if (object == nullptr || codec == nullptr || !codec->has(s, object)) {
        continue;
      }
      net_component_state msg{ .net_id = net_id, .key_hash = key_hash };
      msg.payload = codec->capture(s, object, serialization::default_codec_services());
      session.broadcast(net_message::COMPONENT_STATE, serialize_direct(msg));
    }
    pending_syncs.clear();

    /// [Replicated] script fields: the one automatic lane beyond transforms
    if (collect_script_fields != nullptr) {
      for (const net_object_entry& entry : s.network().entries()) {
        if (!entry.spawn_sent) {
          continue;
        }
        ostd::vector<uint8_t> fields = collect_script_fields(entry.object_id);
        if (fields.empty()) {
          continue;
        }
        net_component_state msg{ .net_id = entry.net_id, .key_hash = kScriptFieldsKey, .payload = std::move(fields) };
        session.broadcast(net_message::COMPONENT_STATE, serialize_direct(msg));
      }
    }

    for (const natural_t net_id : dead) {
      /// the sweep IS the despawn detector: plain destroy_object replicates
      const net_despawn msg{ .net_id = net_id };
      session.broadcast(net_message::DESPAWN, serialize_direct(msg));
      s.network().forget(net_id);
    }

    if (!batch.entries.empty()) {
      session.broadcast(net_message::TRANSFORM_BATCH, serialize_direct(batch));
    }
  }

  void replication::despawn_owned_by(scene& s, uint16_t peer_id) {
    ostd::vector<natural_t> owned;
    for (const net_object_entry& entry : s.network().entries()) {
      if (entry.owner_peer != peer_id) {
        continue;
      }
      const network_component* net = s.get_component<network_component>(entry.object_id);
      if (net != nullptr && net->despawn_on_owner_leave) {
        owned.push_back(entry.object_id);
      }
    }
    for (const natural_t object_id : owned) {
      s.destroy_object(object_id);  // next pass broadcasts the DESPAWN
    }
  }

  /// ---------------------------------------------------------------- client side

  void replication::handle_join_snapshot(scene& s, std::span<const uint8_t> payload) {
    PROFILE_SECTION("replication::handle_join_snapshot");
    net_join_snapshot msg;
    try {
      msg = deserialize_direct<net_join_snapshot>(payload).first;
    } catch (const std::exception& error) {
      CORE_LOG_WARN("[REPL] malformed JOIN_SNAPSHOT dropped: {}", error.what());
      return;
    }
    serialization::scene_parse_result parsed = serialization::parse_scene_binary(msg.scene_bytes);
    if (!parsed.success()) {
      CORE_LOG_WARN("[REPL] join snapshot did not parse: {}", parsed.error);
      return;
    }

    s.destroy_all_non_root_objects();
    s.network().clear();
    interp.clear();
    ostd::map<natural_t, natural_t> remap;
    serialization::instantiate_scene(s, *parsed.document, serialization::default_codec_services(), &remap);

    s.network().role = replication_role::REPLICA;
    for (const net_snapshot_entry& entry : msg.table) {
      auto itr = remap.find(entry.file_id);
      if (itr == remap.end()) {
        CORE_LOG_WARN("[REPL] snapshot table names unknown object {:#x}; skipped", entry.file_id);
        continue;
      }
      s.network().adopt(entry.net_id, itr->second, entry.owner_peer);
    }
    s.revalidate_physics();  // replica dynamics rebuild kinematic

    newest_tick = msg.host_tick;
    newest_arrival = session_now;
    CORE_LOG_INFO("[REPL] joined: {} objects, {} replicated", parsed.document->objects.size(), msg.table.size());
  }

  void replication::handle_spawn(scene& s, std::span<const uint8_t> payload) {
    PROFILE_SECTION("replication::handle_spawn");
    if (s.network().role != replication_role::REPLICA) {
      return;
    }
    net_spawn msg;
    try {
      msg = deserialize_direct<net_spawn>(payload).first;
    } catch (const std::exception&) {
      CORE_LOG_WARN("[REPL] malformed SPAWN dropped");
      return;
    }
    if (s.network().object_of(msg.net_id).has_value()) {
      return;  // snapshot already carried it
    }

    scene_object* parent = nullptr;
    if (const opt<natural_t> parent_object = s.network().object_of(msg.parent_net_id); parent_object.has_value()) {
      parent = s.find_object(*parent_object);
    }
    scene_object& object = s.create_object(std::string(msg.name.begin(), msg.name.end()), parent);

    std::span<const uint8_t> rest{ msg.components };
    for (uint16_t i = 0; i < msg.component_count; ++i) {
      net_component_blob blob;
      try {
        auto [decoded, used] = deserialize_direct<net_component_blob>(rest);
        blob = std::move(decoded);
        rest = rest.subspan(used);
      } catch (const std::exception&) {
        CORE_LOG_WARN("[REPL] truncated SPAWN component list on '{}'", object.name);
        break;
      }
      if (const serialization::component_codec* codec = serialization::find_component_codec(blob.key_hash); codec != nullptr) {
        codec->apply(s, &object, blob.payload, serialization::default_codec_services());
      }
    }

    s.network().adopt(msg.net_id, object.id, msg.owner_peer);
    s.revalidate_physics();
  }

  void replication::handle_despawn(scene& s, std::span<const uint8_t> payload) {
    if (s.network().role != replication_role::REPLICA) {
      return;
    }
    net_despawn msg;
    try {
      msg = deserialize_direct<net_despawn>(payload).first;
    } catch (const std::exception&) {
      return;
    }
    if (const opt<natural_t> object_id = s.network().object_of(msg.net_id); object_id.has_value()) {
      s.destroy_object(*object_id);
    }
    s.network().forget(msg.net_id);
    interp.erase(msg.net_id);
  }

  void replication::handle_transform_batch(scene& s, std::span<const uint8_t> payload) {
    PROFILE_SECTION("replication::handle_transform_batch");
    if (s.network().role != replication_role::REPLICA) {
      return;
    }
    net_transform_batch msg;
    try {
      msg = deserialize_direct<net_transform_batch>(payload).first;
    } catch (const std::exception&) {
      CORE_LOG_WARN("[REPL] malformed TRANSFORM_BATCH dropped");
      return;
    }
    if (msg.host_tick > newest_tick) {
      newest_tick = msg.host_tick;
      newest_arrival = session_now;
    }
    for (const net_transform_entry& entry : msg.entries) {
      if (!s.network().object_of(entry.net_id).has_value()) {
        CORE_LOG_TRACE("[REPL] batch names unknown net id {}; skipped", entry.net_id);
        continue;
      }
      interp[entry.net_id].push({ .tick = msg.host_tick,
                                  .position = entry.position,
                                  .rotation = entry.rotation,
                                  .scale = entry.scale });
    }
  }

  void replication::handle_component_state(scene& s, std::span<const uint8_t> payload) {
    if (s.network().role != replication_role::REPLICA) {
      return;
    }
    net_component_state msg;
    try {
      msg = deserialize_direct<net_component_state>(payload).first;
    } catch (const std::exception&) {
      return;
    }
    const opt<natural_t> object_id = s.network().object_of(msg.net_id);
    if (!object_id.has_value()) {
      return;
    }
    if (msg.key_hash == kScriptFieldsKey) {
      if (apply_script_fields != nullptr) {
        apply_script_fields(*object_id, msg.payload);
      }
      return;
    }
    const serialization::component_codec* codec = serialization::find_component_codec(msg.key_hash);
    scene_object* object = s.find_object(*object_id);
    if (object != nullptr && codec != nullptr) {
      codec->apply(s, object, msg.payload, serialization::default_codec_services());
    }
  }

  void replication::apply_interpolation(scene& s) {
    if (interp.empty()) {
      return;
    }
    PROFILE_SECTION("replication::apply_interpolation");

    /// host-clock estimate advanced by wall time since the newest batch, pulled
    ///  back by the render delay; starved rings clamp to their newest sample (hold)
    const double interval_us = 1'000'000.0 / cfg.snapshot_hz;
    const double render_tick = static_cast<double>(newest_tick) +
      static_cast<double>((session_now - newest_arrival).count()) / interval_us -
      static_cast<double>(cfg.interp_delay.count()) / interval_us;

    for (auto& [net_id, ring] : interp) {
      if (ring.count == 0) {
        continue;
      }
      const opt<natural_t> object_id = s.network().object_of(net_id);
      transform* trs = object_id.has_value() ? s.get_component<transform>(*object_id) : nullptr;
      if (trs == nullptr) {
        continue;
      }
      interp_sample sample;
      ring.sample_at(render_tick, sample);
      trs->local_position = sample.position;
      trs->local_rotation_quat = sample.rotation;
      trs->local_scale = sample.scale;
    }
  }

  void replication::end_session_locally() {
    scene* s = live_scene();
    interp.clear();
    pending_syncs.clear();
    newest_tick = 0;
    snapshot_tick = 0;
    if (s != nullptr) {
      s->network().role = replication_role::NONE;
      s->network().clear();
      /// replicas stay as local objects; their bodies revalidate to authored settings
      s->revalidate_physics();
    }
  }

  /// ---------------------------------------------------------------- interp ring

  void replication::interp_ring::push(const interp_sample& sample) {
    if (count < samples.size()) {
      samples[count++] = sample;
      return;
    }
    for (size_t i = 1; i < samples.size(); ++i) {
      samples[i - 1] = samples[i];
    }
    samples.back() = sample;
  }

  const replication::interp_sample* replication::interp_ring::newest() const {
    return count > 0 ? &samples[count - 1] : nullptr;
  }

  void replication::interp_ring::sample_at(double tick, interp_sample& out) const {
    OTHER_ASSERT(count > 0, "sample_at on an empty ring");
    if (tick <= static_cast<double>(samples[0].tick)) {
      out = samples[0];
      return;
    }
    if (tick >= static_cast<double>(samples[count - 1].tick)) {
      out = samples[count - 1];
      return;
    }
    for (size_t i = 1; i < count; ++i) {
      const interp_sample& a = samples[i - 1];
      const interp_sample& b = samples[i];
      if (tick > static_cast<double>(b.tick)) {
        continue;
      }
      const double span = static_cast<double>(b.tick - a.tick);
      const float t = span > 0.0 ? static_cast<float>((tick - static_cast<double>(a.tick)) / span) : 1.f;
      out.tick = b.tick;
      out.position = glm::mix(a.position, b.position, t);
      out.rotation = glm::slerp(a.rotation, b.rotation, t);
      out.scale = glm::mix(a.scale, b.scale, t);
      return;
    }
    out = samples[count - 1];
  }

}  // namespace other

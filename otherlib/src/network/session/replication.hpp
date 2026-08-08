/**
 * \file network/session/replication.hpp
 **/
#ifndef OTHERLIB_NETWORK_SESSION_REPLICATION_HPP
#define OTHERLIB_NETWORK_SESSION_REPLICATION_HPP

#include <array>
#include <functional>

#include "core/defines.hpp"

#include "scene/scene.hpp"

#include "network/session/network_session.hpp"

namespace other {

  struct replication_config {
    double snapshot_hz = 20.0;
    microseconds interp_delay{ 100'000 };
    /// squared per-term local-TRS delta below which a transform is not dirty
    float transform_epsilon = 1e-6f;
  };

  /// host-authoritative replication over the default session — the shipped default
  ///  configuration, not a protocol law (D19): owner_peer/IsMine/COMPONENT_STATE/
  ///  the handler registry are the seams other authority models build against.
  ///  the owner forwards session events and ticks it; nothing here touches the
  ///  session observer, so it composes with the driver glue and with tests alike
  class replication {
   public:
    replication(network_session& session, std::function<scene*()> scene_source, const replication_config& cfg = {});

    void on_session_event(session_event ev, uint16_t arg);
    void tick(microseconds now);

    uint64_t host_tick() const { return snapshot_tick; }

    /// host-only: register + replicate an object (SPAWN goes out on the next pass)
    bool spawn_object(natural_t object_id, uint16_t owner_peer = 0);
    /// host-only: push one component's current state to every replica
    bool sync_component(natural_t object_id, natural_t key_hash);
    bool is_mine(natural_t object_id) const;

    /// the [Replicated] script-field lane: one automatic COMPONENT_STATE stream
    ///  keyed kScriptFieldsKey. the hooks keep scripting out of this module —
    ///  the glue wires C#, tests wire fakes. collect returns the dirty-field blob
    ///  (empty = clean); apply writes it on the replica. format is the collector's
    constexpr static natural_t kScriptFieldsKey = FNV("script-fields");
    using script_field_collector = std::function<ostd::vector<uint8_t>(natural_t object_id)>;
    using script_field_applier = std::function<void(natural_t object_id, std::span<const uint8_t> payload)>;
    void set_script_field_hooks(script_field_collector collect, script_field_applier apply) {
      collect_script_fields = std::move(collect);
      apply_script_fields = std::move(apply);
    }

   private:
    struct interp_sample {
      uint64_t tick = 0;
      glm::vec3 position{ 0.f };
      glm::quat rotation{};
      glm::vec3 scale{ 1.f };
    };
    /// ring of the newest 8 samples, oldest evicted; ticks arrive monotonic on
    ///  reliable-ordered links
    struct interp_ring {
      std::array<interp_sample, 8> samples{};
      size_t count = 0;

      void push(const interp_sample& sample);
      const interp_sample* newest() const;
      /// bracketing pair for a tick timeline position; clamps at both ends
      void sample_at(double tick, interp_sample& out) const;
    };

    network_session& session;
    std::function<scene*()> scene_source;
    replication_config cfg;

    microseconds session_now{ 0 };
    microseconds last_snapshot{ 0 };
    uint64_t snapshot_tick = 0;

    /// client host-clock estimate: newest batch tick + its arrival instant
    uint64_t newest_tick = 0;
    microseconds newest_arrival{ 0 };

    ostd::map<natural_t, interp_ring> interp;
    ostd::vector<std::pair<natural_t, natural_t>> pending_syncs;  // net_id, key_hash
    script_field_collector collect_script_fields;
    script_field_applier apply_script_fields;

    // host
    void sweep_authored(scene& s);
    void snapshot_pass(scene& s);
    void send_join_snapshot(scene& s, uint16_t peer_id);
    void despawn_owned_by(scene& s, uint16_t peer_id);

    // client
    void handle_join_snapshot(scene& s, std::span<const uint8_t> payload);
    void handle_spawn(scene& s, std::span<const uint8_t> payload);
    void handle_despawn(scene& s, std::span<const uint8_t> payload);
    void handle_transform_batch(scene& s, std::span<const uint8_t> payload);
    void handle_component_state(scene& s, std::span<const uint8_t> payload);
    void apply_interpolation(scene& s);
    void end_session_locally();

    scene* live_scene() const { return scene_source != nullptr ? scene_source() : nullptr; }
  };

}  // namespace other

#endif  // OTHERLIB_NETWORK_SESSION_REPLICATION_HPP

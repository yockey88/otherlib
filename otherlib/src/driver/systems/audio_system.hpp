/**
 * \file driver/systems/audio_system.hpp
 **/
#ifndef OTHERLIB_DRIVER_SYSTEMS_AUDIO_SYSTEM_HPP
#define OTHERLIB_DRIVER_SYSTEMS_AUDIO_SYSTEM_HPP

#include <functional>

#include "driver/systems/core_system.hpp"

namespace other {

  class scene;
  class audio_environment;

  /// asset id -> registry hash (path_hash); injected so headless tests can
  ///  reconcile against clips keyed however they like (identity fn)
  using asset_hash_fn = std::function<natural_t(natural_t)>;

  /// the reconciler's memory between ticks: which voice each object owns. entity
  ///  destruction (including play/stop's teardown-rebuild) leaves voices with no
  ///  surviving component to carry the handle — the sweep over this map is what
  ///  stops them. tree ids are high-water so reuse is rare; the divergence guard
  ///  in the reconciler covers the wrap case
  struct audio_reconcile_state {
    ostd::map<natural_t, voice_id> bound;
  };

  /// desired-state reconciliation: components (and one-shots) describe what should
  ///  be audible, this diffs that against live voices. play/stop restore, hot
  ///  reload (revision compare), entity deletion, and failed loads are all the
  ///  same code path. gameplay sources are silent while the scene isn't playing
  void reconcile_scene_audio(scene* active_scene, audio_environment* env, const asset_hash_fn& hash_of_asset, double dt, audio_reconcile_state& state);

  /// ticks right after the scene system so voices read the frame's final world
  ///  transforms; owns pump-mode time advancement and voice reconciliation. the
  ///  audio_environment subsystem owns the device/engine/registry state itself
  class OTHER_CLASS audio_system : public core_system<audio_system> {
   public:
    audio_system(driver* driver_instance)
        : core_system(driver_instance, driver_system_type::AUDIO_DRIVER_SYSTEM) {}
    virtual ~audio_system() = default;

    std::string name() const override { return "Audio System"; }

    void initialize(driver_kernel* kernel) override;
    void tick(driver_kernel* kernel, double dt) override;
    void shutdown(driver_kernel* kernel) override;

   private:
    audio_reconcile_state reconcile_state;
  };

}  // namespace other

#endif  // OTHERLIB_DRIVER_SYSTEMS_AUDIO_SYSTEM_HPP

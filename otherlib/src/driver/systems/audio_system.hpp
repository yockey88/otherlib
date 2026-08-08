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

  /// per-tick voice ownership map; sweeping it stops voices whose owning entity
  ///  was destroyed (tree ids are high-water, so id reuse is rare)
  struct audio_reconcile_state {
    ostd::map<natural_t, voice_id> bound;
  };

  /// diffs desired component audio state against live voices; play/stop, hot-reload,
  ///  deletion, and failed loads share this path. silent while scene isn't playing
  void reconcile_scene_audio(scene* active_scene, audio_environment* env, const asset_hash_fn& hash_of_asset, double dt, audio_reconcile_state& state);

  /// ticks after scene system so voices read final world transforms; owns pump-mode
  ///  timing + reconciliation. audio_environment owns the device/engine state
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

/**
 * \file driver/systems/audio_system.cpp
 **/
#include "driver/systems/audio_system.hpp"

#include "audio/audio_environment.hpp"

namespace other {

  void audio_system::initialize(driver_kernel* kernel) {
    CORE_LOG_DEBUG("Audio system initialized (environment {})",
                   subsystem<audio_environment>::inert ? "inert" : "active");
  }

  void audio_system::tick(driver_kernel* kernel, double dt) {
    if (subsystem<audio_environment>::inert) {
      return;
    }
    audio_environment* env = subsystem<audio_environment>::get();
    if (env == nullptr || !env->is_initialized()) {
      return;
    }

    if (env->pump_mode()) {
      env->pump(dt);
    }
  }

  void audio_system::shutdown(driver_kernel* kernel) {
    if (subsystem<audio_environment>::inert) {
      return;
    }
    audio_environment* env = subsystem<audio_environment>::get();
    if (env == nullptr || !env->is_initialized()) {
      return;
    }

    /// voices die here, while clips are still registered; the device itself closes
    ///  later in subsystem_registry::shutdown_all (reverse dependency order)
    env->stop_all_voices();
  }

}  // namespace other

/**
 * \file driver/systems/audio_system.hpp
 **/
#ifndef OTHERLIB_DRIVER_SYSTEMS_AUDIO_SYSTEM_HPP
#define OTHERLIB_DRIVER_SYSTEMS_AUDIO_SYSTEM_HPP

#include "driver/systems/core_system.hpp"

namespace other {

  /// ticks right after the scene system so voices read the frame's final world
  ///  transforms; owns pump-mode time advancement and (with the scene half of
  ///  audio-1) desired-state voice reconciliation. the audio_environment
  ///  subsystem owns the device/engine/registry state itself
  class OTHER_CLASS audio_system : public core_system<audio_system> {
   public:
    audio_system(driver* driver_instance)
        : core_system(driver_instance, driver_system_type::AUDIO_DRIVER_SYSTEM) {}
    virtual ~audio_system() = default;

    std::string name() const override { return "Audio System"; }

    void initialize(driver_kernel* kernel) override;
    void tick(driver_kernel* kernel, double dt) override;
    void shutdown(driver_kernel* kernel) override;
  };

}  // namespace other

#endif  // OTHERLIB_DRIVER_SYSTEMS_AUDIO_SYSTEM_HPP

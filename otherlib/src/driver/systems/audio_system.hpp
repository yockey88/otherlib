/**
 * \file driver/systems/audio_system.hpp
 **/
#ifndef OTHERLIB_DRIVER_SYSTEMS_AUDIO_SYSTEM_HPP
#define OTHERLIB_DRIVER_SYSTEMS_AUDIO_SYSTEM_HPP

#include "driver/systems/core_system.hpp"

namespace other {

  class audio_system : public core_system<audio_system> {
   public:
    audio_system(driver* driver_instance)
        : core_system<audio_system>(driver_instance, driver_system_type::AUDIO_DRIVER_SYSTEM) {}
    ~audio_system() override = default;

    std::string name() const override { return "Audio System"; }

    void initialize(driver_kernel* kernel) override;
    void tick(driver_kernel* kernel, double dt) override;
    void shutdown(driver_kernel* kernel) override;

   private:
    struct loaded_device {
      natural_t id = 0;
      // ref<audio_device> device;
    };
    struct loaded_sound {
      natural_t id = 0;
      // ref<audio_sound> sound;
    };

    std::vector<loaded_device> devices;
    std::vector<loaded_sound> sounds;
  };

}  // namespace other

#endif  // OTHERLIB_DRIVER_SYSTEMS_AUDIO_SYSTEM_HPP
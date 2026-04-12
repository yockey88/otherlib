/**
 * \file driver/systems/scene_system.hpp
 **/
#ifndef OTHERLIB_DRIVER_SYSTEMS_SCENE_SYSTEM_HPP
#define OTHERLIB_DRIVER_SYSTEMS_SCENE_SYSTEM_HPP

#include "driver/systems/driver_system.hpp"

namespace other {

  class scene_system : public core_system<scene_system> {
   public:
    scene_system(driver* driver_instance)
        : core_system(driver_instance, driver_system_type::SCENE_DRIVER_SYSTEM) {}
    virtual ~scene_system() = default;

    void initialize() override;
    void tick(float dt) override;
    void shutdown() override;

   private:
  };

}  // namespace other

#endif  // OTHERLIB_DRIVER_SYSTEMS_SCENE_SYSTEM_HPP
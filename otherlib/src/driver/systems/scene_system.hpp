/**
 * \file driver/systems/scene_system.hpp
 **/
#ifndef OTHERLIB_DRIVER_SYSTEMS_SCENE_SYSTEM_HPP
#define OTHERLIB_DRIVER_SYSTEMS_SCENE_SYSTEM_HPP

#include "core/value.hpp"

#include "driver/driver_kernel.hpp"

namespace other {

  class scene_system : public core_system<scene_system> {
   public:
    scene_system(driver* driver_instance)
        : core_system(driver_instance, driver_system_type::SCENE_DRIVER_SYSTEM) {}
    virtual ~scene_system() = default;

    std::string name() const override { return "Scene Driver System"; }

    void initialize(driver_kernel* kernel) override;
    void tick(driver_kernel* kernel, float dt) override;
    void shutdown(driver_kernel* kernel) override;

   private:
    void handle_scene_load_empty_event(const value& data);
    void handle_scene_load_event(const value& data);
    void handle_scene_unload_event(const value& data);
    void handle_scene_info_event(const value& data);
    void handle_scene_playback_command_event(const value& data);
  };

}  // namespace other

#endif  // OTHERLIB_DRIVER_SYSTEMS_SCENE_SYSTEM_HPP
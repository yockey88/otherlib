/**
 * \file vm/driver_interface.cpp
 **/
#include "vm/driver_interface.hpp"

#include "driver/driver.hpp"
#include "driver/systems/scene_system.hpp"

namespace other {

  void driver_interface::set_scene_by_id(driver* driver_instance, natural_t scene_id) {
    auto& scene_sys = driver_instance->get_kernel().get_core_system<scene_system>();
    scene_sys.set_scene_to_active(scene_id);
  }

}  // namespace other
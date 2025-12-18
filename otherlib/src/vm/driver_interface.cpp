/**
 * \file vm/driver_interface.cpp
 **/
#include "vm/driver_interface.hpp"

#include "driver/driver.hpp"
#include "vm/other_device.hpp"

namespace other {

  void driver_interface::set_scene_by_id(driver* driver_instance, natural_t scene_id) {
    driver_instance->set_scene_to_active(scene_id);
  }

}  // namespace other
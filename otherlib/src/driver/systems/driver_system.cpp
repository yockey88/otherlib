/**
 * \file driver/systems/driver_system.cpp
 **/
#include "driver/systems/driver_system.hpp"

namespace other {

  driver& driver_system::get_driver() {
    OTHER_ASSERT(driver_instance != nullptr, "Driver system is not associated with a driver.");
    return *driver_instance;
  }

  void driver_plugin::initialize(driver_kernel* kernel) {
    if (!active()) {
      on_initialize();
      set_active(true);
    }
  }

  void driver_plugin::tick(driver_kernel* kernel, double dt) {
    if (active()) {
      on_tick(dt);
    }
  }

  void driver_plugin::shutdown(driver_kernel* kernel) {
    if (active()) {
      on_shutdown();
      set_active(false);
    }
  }

}  // namespace other
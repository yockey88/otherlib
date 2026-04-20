/**
 * \file driver/systems/driver_plugin.cpp
 **/
#include "driver/systems/driver_plugin.hpp"

namespace other {

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
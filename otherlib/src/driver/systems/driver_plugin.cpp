/**
 * \file driver/systems/driver_plugin.cpp
 **/
#include "driver/systems/driver_plugin.hpp"

namespace other {

  void driver_plugin::initialize(driver_kernel* kernel) {
    OTHER_ASSERT(kernel != nullptr, "Kernel pointer is null in driver_plugin::initialize.");
    if (!active()) {
      on_initialize(*kernel);
      set_active(true);
    }
  }

  void driver_plugin::tick(driver_kernel* kernel, double dt) {
    OTHER_ASSERT(kernel != nullptr, "Kernel pointer is null in driver_plugin::tick.");
    if (active()) {
      on_tick(*kernel, dt);
    }
  }

  void driver_plugin::shutdown(driver_kernel* kernel) {
    OTHER_ASSERT(kernel != nullptr, "Kernel pointer is null in driver_plugin::shutdown.");
    if (active()) {
      on_shutdown(*kernel);
      set_active(false);
    }
  }

}  // namespace other
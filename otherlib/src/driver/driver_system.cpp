/**
 * \file driver/systems/driver_system.cpp
 **/
#include "driver/driver_system.hpp"

namespace other {

  driver& driver_system::get_driver() {
    OTHER_ASSERT(driver_instance != nullptr, "Driver system is not associated with a driver.");
    return *driver_instance;
  }

}  // namespace other
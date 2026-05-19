/**
 * \file driver/systems/audio_system.cpp
 **/
#include "driver/systems/audio_system.hpp"

namespace other {

  void audio_system::initialize(driver_kernel* kernel) {
    OTHER_ASSERT(kernel != nullptr, "Null driver kernel initializing audio system");
  }

  void audio_system::tick(driver_kernel* kernel, double dt) {
    OTHER_ASSERT(kernel != nullptr, "Null driver kernel on audio system tick");
  }

  void audio_system::shutdown(driver_kernel* kernel) {
    OTHER_ASSERT(kernel != nullptr, "Null driver kernel shutting down audio system");
  }

}  // namespace other
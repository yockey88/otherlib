/**
 * \file driver/systems/scene_system.cpp
 **/
#include "driver/systems/scene_system.hpp"

#include "scripting/scene_interface.hpp"

namespace other {

  void scene_system::initialize() {
    scene_interface::initialize(&get_driver());
  }

  void scene_system::tick(float dt) {
    // CORE_LOG_INFO("Ticking Scene Driver System with dt = {} seconds.", dt);
  }

  void scene_system::shutdown() {
    CORE_LOG_INFO("Shutting down Scene Driver System.");
  }

}  // namespace other
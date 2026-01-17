/**
 * \file physics/physics_api.cpp
 **/
#include "physics/physics_api.hpp"

#include "physics_world/physics_world.hpp"

namespace other {

  void physics_api::initialize(const config_table& configuration) {
    on_initialize(configuration);
  }

  void physics_api::shutdown() {
    on_shutdown();
  }

}  // namespace other
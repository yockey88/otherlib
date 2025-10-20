/**
 * \file physics/physics_environment.hpp
 **/
#ifndef OTHER_PHYSICS_PHYSICS_PHYSICS_ENVIRONMENT_HPP
#define OTHER_PHYSICS_PHYSICS_PHYSICS_ENVIRONMENT_HPP

#include "core/config_table.hpp"
#include "core/subsystem.hpp"

namespace other {

  class physics_environment : public subsystem<physics_environment> {
   public:
    physics_environment() = default;
    virtual ~physics_environment() = default;

    void initialize_physics_environment(const config_table& configuration);
    void shutdown_physics_environment();
  };

}  // namespace other

OTHER_SUBSYSTEM(other::physics_environment);

#endif  // OTHER_PHYSICS_PHYSICS_PHYSICS_ENVIRONMENT_HPP
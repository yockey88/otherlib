// /**
//  * \file math-physics-driver.hpp
//  **/
#ifndef OTHER_MATH_PHYSICS_DRIVER_HPP
#define OTHER_MATH_PHYSICS_DRIVER_HPP

#include "driver/driver.hpp"

namespace other {

  class OTHER_CLASS math_physics_driver : public driver {
   public:
    math_physics_driver(const config_table& config)
        : driver(config) {}
    virtual ~math_physics_driver() = default;

    void on_initialize(const command_line& cmd) override;
    void run() override;
    void on_shutdown() override;
  };

}  // namespace other

OTHER_DRIVER(other::math_physics_driver)

#endif  // OTHER_MATH_PHYSICS_DRIVER_HPP
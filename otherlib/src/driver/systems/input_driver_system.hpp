/**
 * \file driver/systems/input_driver_system.hpp
 **/
#ifndef OTHERLIB_DRIVER_SYSTEMS_INPUT_DRIVER_SYSTEM_HPP
#define OTHERLIB_DRIVER_SYSTEMS_INPUT_DRIVER_SYSTEM_HPP

#include "input/input_system.hpp"

#include "driver/systems/core_system.hpp"
namespace other {

  class OTHER_CLASS input_driver_system : public core_system<input_driver_system> {
   public:
    input_driver_system(driver* driver_instance)
        : core_system(driver_instance, static_cast<uint32_t>(driver_system_type::INPUT_DRIVER_SYSTEM)) {}

    std::string name() const override { return "Input Driver System"; }

    void initialize(driver_kernel* kernel) override;
    void tick(driver_kernel* kernel, double dt) override;
    void shutdown(driver_kernel* kernel) override;

   private:
    void handle_input_event(driver_kernel* kernel, const input_state_change_event& event);

    input_map get_driver_input_map();
  };

}  // namespace other

#endif  // OTHERLIB_DRIVER_SYSTEMS_INPUT_DRIVER_SYSTEM_HPP